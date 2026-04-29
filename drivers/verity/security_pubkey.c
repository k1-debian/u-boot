#include <common.h>
#include <asm/byteorder.h>
#include <asm/errno.h>
#include <malloc.h>
#include <security_pubkey.h>
#include <sha256.h>

#define SECURITY_RSA_PUBKEY_MAGIC	0x52504b31U
#define SECURITY_RSA_PUBKEY_VERSION	1U

struct security_rsa_pubkey_storage_v1 {
	unsigned int magic;
	unsigned short version;
	unsigned short header_size;
	unsigned int algo;
	unsigned int key_bits;
	unsigned int exponent;
	unsigned int n0inv;
	unsigned char modulus[SECURITY_RSA2048_BYTES];
	unsigned char rr[SECURITY_RSA2048_BYTES];
} __attribute__((packed));

static unsigned int security_load_be32(const unsigned char *buf)
{
	return ((unsigned int)buf[0] << 24) |
	       ((unsigned int)buf[1] << 16) |
	       ((unsigned int)buf[2] << 8) |
	       (unsigned int)buf[3];
}

static unsigned int security_modinv32(unsigned int a)
{
	unsigned int x;

	x = a;
	x *= 2 - a * x;
	x *= 2 - a * x;
	x *= 2 - a * x;
	x *= 2 - a * x;
	x *= 2 - a * x;
	return 0U - x;
}

static void security_rsa_subtract_modulus(
	const struct security_rsa_public_key *key, unsigned int num[])
{
	long long acc = 0;
	unsigned int i;

	for (i = 0; i < key->len; i++) {
		acc += (unsigned long long)num[i] - key->modulus[i];
		num[i] = (unsigned int)acc;
		acc >>= 32;
	}
}

static int security_rsa_greater_equal_modulus(
	const struct security_rsa_public_key *key, unsigned int num[])
{
	int i;

	for (i = key->len - 1; i >= 0; i--) {
		if (num[i] < key->modulus[i])
			return 0;
		if (num[i] > key->modulus[i])
			return 1;
	}

	return 1;
}

static void security_rsa_lshift1_mod(const struct security_rsa_public_key *key,
				     unsigned int value[])
{
	unsigned int i;
	unsigned int carry = 0;
	unsigned int new_carry;

	for (i = 0; i < key->len; i++) {
		new_carry = value[i] >> 31;
		value[i] = (value[i] << 1) | carry;
		carry = new_carry;
	}

	if (carry || security_rsa_greater_equal_modulus(key, value))
		security_rsa_subtract_modulus(key, value);
}

static void security_rsa_compute_rr(struct security_rsa_public_key *key)
{
	unsigned int i;

	for (i = 0; i < key->len; i++)
		key->rr[i] = 0;

	key->rr[0] = 1;
	for (i = 0; i < key->len * 64; i++)
		security_rsa_lshift1_mod(key, key->rr);
}

static int security_base64_value(char ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A';
	if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 26;
	if (ch >= '0' && ch <= '9')
		return ch - '0' + 52;
	if (ch == '+')
		return 62;
	if (ch == '/')
		return 63;
	return -1;
}

static int security_base64_decode(const char *src, unsigned int src_len,
				  unsigned char *dst,
				  unsigned int *dst_len)
{
	unsigned int out_len = 0;
	unsigned int i = 0;

	if (!src || !dst || !dst_len)
		return -EINVAL;

	while (i < src_len) {
		int vals[4];
		unsigned int j;
		unsigned int pad = 0;

		for (j = 0; j < 4; j++) {
			while (i < src_len &&
			       (src[i] == '\r' || src[i] == '\n' ||
				src[i] == ' ' || src[i] == '\t'))
				i++;

			if (i >= src_len)
				return j ? -EINVAL : 0;

			if (src[i] == '=') {
				vals[j] = 0;
				pad++;
				i++;
				continue;
			}

			vals[j] = security_base64_value(src[i]);
			if (vals[j] < 0)
				return -EINVAL;
			i++;
		}

		if (out_len + 3 > 1024)
			return -EMSGSIZE;

		dst[out_len++] = (vals[0] << 2) | (vals[1] >> 4);
		if (pad < 2)
			dst[out_len++] = (vals[1] << 4) | (vals[2] >> 2);
		if (!pad)
			dst[out_len++] = (vals[2] << 6) | vals[3];
	}

	*dst_len = out_len;
	return 0;
}

static int security_asn1_read_tlv(const unsigned char **cursor,
				  const unsigned char *end,
				  unsigned int *tag,
				  const unsigned char **value,
				  unsigned int *length)
{
	unsigned int len;
	unsigned int i;
	unsigned int len_len;

	if (!cursor || !*cursor || !tag || !value || !length || *cursor >= end)
		return -EINVAL;

	*tag = *(*cursor)++;
	if (*cursor >= end)
		return -EINVAL;

	len = *(*cursor)++;
	if (len & 0x80) {
		len_len = len & 0x7f;
		if (!len_len || len_len > 4 || *cursor + len_len > end)
			return -EINVAL;

		len = 0;
		for (i = 0; i < len_len; i++)
			len = (len << 8) | *(*cursor)++;
	}

	if (*cursor + len > end)
		return -EINVAL;

	*value = *cursor;
	*length = len;
	*cursor += len;
	return 0;
}

static int security_parse_rsa_pubkey_der(const unsigned char *der,
					 unsigned int der_len,
					 int is_spki,
					 struct security_rsa_public_key *out_key)
{
	const unsigned char *cursor;
	const unsigned char *end;
	const unsigned char *value;
	const unsigned char *modulus;
	unsigned int tag;
	unsigned int length;
	unsigned int modulus_len;
	unsigned int exponent = 0;
	unsigned int i;

	if (!der || !der_len || !out_key)
		return -EINVAL;

	cursor = der;
	end = der + der_len;
	if (security_asn1_read_tlv(&cursor, end, &tag, &value, &length))
		return -EINVAL;
	if (tag != 0x30)
		return -EINVAL;

	cursor = value;
	end = value + length;
	if (is_spki) {
		const unsigned char *bit_string;
		unsigned int bit_len;

		if (security_asn1_read_tlv(&cursor, end, &tag, &value, &length))
			return -EINVAL;
		if (tag != 0x30)
			return -EINVAL;

		if (security_asn1_read_tlv(&cursor, end, &tag, &bit_string,
					  &bit_len))
			return -EINVAL;
		if (tag != 0x03 || !bit_len || bit_string[0] != 0)
			return -EINVAL;

		cursor = bit_string + 1;
		end = bit_string + bit_len;
		if (security_asn1_read_tlv(&cursor, end, &tag, &value, &length))
			return -EINVAL;
		if (tag != 0x30)
			return -EINVAL;
		cursor = value;
		end = value + length;
	}

	if (security_asn1_read_tlv(&cursor, end, &tag, &modulus, &modulus_len))
		return -EINVAL;
	if (tag != 0x02)
		return -EINVAL;

	if (modulus_len == SECURITY_RSA2048_BYTES + 1 && modulus[0] == 0) {
		modulus++;
		modulus_len--;
	}
	if (modulus_len != SECURITY_RSA2048_BYTES)
		return -EINVAL;

	if (security_asn1_read_tlv(&cursor, end, &tag, &value, &length))
		return -EINVAL;
	if (tag != 0x02 || !length || length > 4)
		return -EINVAL;

	for (i = 0; i < length; i++)
		exponent = (exponent << 8) | value[i];
	if (exponent != 65537U)
		return -EINVAL;

	memset(out_key, 0, sizeof(*out_key));
	out_key->len = SECURITY_RSA2048_WORDS;
	for (i = 0; i < SECURITY_RSA2048_WORDS; i++)
		out_key->modulus[i] =
			security_load_be32(modulus + SECURITY_RSA2048_BYTES -
					   (i + 1) * 4);

	out_key->n0inv = security_modinv32(out_key->modulus[0]);
	security_rsa_compute_rr(out_key);
	return 0;
}

static int security_parse_rsa_pubkey_pem(
	const struct security_pubkey *pubkey,
	struct security_rsa_public_key *out_key)
{
	static const char begin_spki[] = "-----BEGIN PUBLIC KEY-----";
	static const char end_spki[] = "-----END PUBLIC KEY-----";
	static const char begin_rsa[] = "-----BEGIN RSA PUBLIC KEY-----";
	static const char end_rsa[] = "-----END RSA PUBLIC KEY-----";
	char pem[1024];
	unsigned char der[1024];
	const char *data;
	const char *begin;
	const char *end;
	unsigned int der_len = 0;
	int is_spki = 1;
	int ret;

	if (!pubkey || !pubkey->data || !out_key)
		return -EINVAL;

	if (pubkey->data_len >= sizeof(pem))
		return -EMSGSIZE;

	memcpy(pem, pubkey->data, pubkey->data_len);
	pem[pubkey->data_len] = '\0';
	data = pem;
	begin = strstr(data, begin_spki);
	end = strstr(data, end_spki);
	if (!begin || !end) {
		begin = strstr(data, begin_rsa);
		end = strstr(data, end_rsa);
		is_spki = 0;
	}
	if (!begin || !end || end <= begin)
		return -EINVAL;

	begin = strchr(begin, '\n');
	if (!begin)
		return -EINVAL;
	begin++;

	ret = security_base64_decode(begin, end - begin, der, &der_len);
	if (ret)
		return ret;

	return security_parse_rsa_pubkey_der(der, der_len, is_spki, out_key);
}

int security_pubkey_parse_rsa(const struct security_pubkey *pubkey,
			      struct security_rsa_public_key *out_key)
{
	const struct security_rsa_pubkey_storage_v1 *key;
	const unsigned int *modulus_words;
	const unsigned int *rr_words;
	unsigned int i;
	int ret;

	if (!pubkey || !out_key || !pubkey->data)
		return -EINVAL;

	key = pubkey->data;
	if (pubkey->data_len >= sizeof(*key) &&
	    key->magic == SECURITY_RSA_PUBKEY_MAGIC) {
		if (key->version != SECURITY_RSA_PUBKEY_VERSION ||
		    key->header_size != sizeof(*key))
			return -EPROTONOSUPPORT;

		if (key->algo != SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1 ||
		    pubkey->algo != SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1)
			return -EPROTONOSUPPORT;

		if (key->key_bits != 2048 || key->exponent != 65537U)
			return -EINVAL;

		out_key->len = SECURITY_RSA2048_WORDS;
		out_key->n0inv = key->n0inv;
		modulus_words = (const unsigned int *)key->modulus;
		rr_words = (const unsigned int *)key->rr;
		for (i = 0; i < SECURITY_RSA2048_WORDS; i++) {
			out_key->modulus[i] =
				be32_to_cpu(modulus_words[SECURITY_RSA2048_WORDS - 1 - i]);
			out_key->rr[i] =
				be32_to_cpu(rr_words[SECURITY_RSA2048_WORDS - 1 - i]);
		}
		return 0;
	}

	ret = security_parse_rsa_pubkey_pem(pubkey, out_key);
	if (!ret)
		return 0;

	if (pubkey->data_len && *(const unsigned char *)pubkey->data == 0x30) {
		ret = security_parse_rsa_pubkey_der(pubkey->data, pubkey->data_len,
						   1, out_key);
		if (!ret)
			return 0;

		ret = security_parse_rsa_pubkey_der(pubkey->data, pubkey->data_len,
						   0, out_key);
		if (!ret)
			return 0;
	}

	return -EINVAL;
}

int security_pubkey_dup(const struct security_pubkey *src,
			struct security_pubkey *out)
{
	void *data;

	if (!src || !out || !src->data || !src->data_len)
		return -EINVAL;

	memset(out, 0, sizeof(*out));

	data = malloc(src->data_len);
	if (!data)
		return -ENOMEM;

	memcpy(data, src->data, src->data_len);
	out->key_id = src->key_id;
	out->algo = src->algo;
	out->data = data;
	out->data_len = src->data_len;
	out->flags = SECURITY_PUBKEY_FLAG_OWNS_DATA;
	return 0;
}

void security_pubkey_release(struct security_pubkey *pubkey)
{
	if (!pubkey)
		return;

	if ((pubkey->flags & SECURITY_PUBKEY_FLAG_OWNS_DATA) && pubkey->data)
		free((void *)pubkey->data);

	memset(pubkey, 0, sizeof(*pubkey));
}

static void security_rsa_montgomery_mul_add_step(
	const struct security_rsa_public_key *key,
	unsigned int result[], unsigned int a,
	const unsigned int b[])
{
	unsigned long long acc_a, acc_b;
	unsigned int d0;
	unsigned int i;

	acc_a = (unsigned long long)a * b[0] + result[0];
	d0 = (unsigned int)acc_a * key->n0inv;
	acc_b = (unsigned long long)d0 * key->modulus[0] + (unsigned int)acc_a;
	for (i = 1; i < key->len; i++) {
		acc_a = (acc_a >> 32) + (unsigned long long)a * b[i] + result[i];
		acc_b = (acc_b >> 32) + (unsigned long long)d0 * key->modulus[i] +
			(unsigned int)acc_a;
		result[i - 1] = (unsigned int)acc_b;
	}

	acc_a = (acc_a >> 32) + (acc_b >> 32);
	result[i - 1] = (unsigned int)acc_a;
	if (acc_a >> 32)
		security_rsa_subtract_modulus(key, result);
}

static void security_rsa_montgomery_mul(
	const struct security_rsa_public_key *key,
	unsigned int result[], unsigned int a[],
	const unsigned int b[])
{
	unsigned int i;

	for (i = 0; i < key->len; i++)
		result[i] = 0;
	for (i = 0; i < key->len; i++)
		security_rsa_montgomery_mul_add_step(key, result, a[i], b);
}

static int security_rsa_pow_mod(const struct security_rsa_public_key *key,
				unsigned int *inout)
{
	unsigned int val[SECURITY_RSA2048_WORDS];
	unsigned int acc[SECURITY_RSA2048_WORDS];
	unsigned int tmp[SECURITY_RSA2048_WORDS];
	unsigned int *result = tmp;
	unsigned int i;
	unsigned int *ptr;

	for (i = 0, ptr = inout + key->len - 1; i < key->len; i++, ptr--)
		val[i] = be32_to_cpu(*ptr);

	security_rsa_montgomery_mul(key, acc, val, key->rr);
	for (i = 0; i < 16; i += 2) {
		security_rsa_montgomery_mul(key, tmp, acc, acc);
		security_rsa_montgomery_mul(key, acc, tmp, tmp);
	}
	security_rsa_montgomery_mul(key, result, acc, val);

	if (security_rsa_greater_equal_modulus(key, result))
		security_rsa_subtract_modulus(key, result);

	for (i = key->len - 1, ptr = inout; (int)i >= 0; i--, ptr++)
		*ptr = cpu_to_be32(result[i]);

	return 0;
}

int security_pubkey_verify_rsa2048_sha256(
	const struct security_pubkey *pubkey,
	const unsigned char *sig, unsigned int sig_len,
	const unsigned char *digest, unsigned int digest_len)
{
	static const unsigned char sha256_prefix[] = {
		0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
		0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
		0x00, 0x04, 0x20,
	};
	struct security_rsa_public_key rsa_key;
	unsigned char em[SECURITY_RSA2048_BYTES];
	unsigned int pad_len;
	unsigned int i;
	int ret;

	if (!pubkey || !sig || !digest)
		return -EINVAL;

	if (digest_len != SHA256_SUM_LEN)
		return -EINVAL;

	ret = security_pubkey_parse_rsa(pubkey, &rsa_key);
	if (ret)
		return ret;

	if (sig_len != SECURITY_RSA2048_BYTES)
		return -EINVAL;

	memcpy(em, sig, sizeof(em));
	security_rsa_pow_mod(&rsa_key, (unsigned int *)em);

	if (em[0] != 0x00 || em[1] != 0x01)
		return -EACCES;

	for (i = 2; i < sizeof(em); i++) {
		if (em[i] == 0x00)
			break;
		if (em[i] != 0xff)
			return -EACCES;
	}

	if (i < 10 || i >= sizeof(em))
		return -EACCES;

	pad_len = i + 1;
	if (pad_len + sizeof(sha256_prefix) + SHA256_SUM_LEN != sizeof(em))
		return -EACCES;

	if (memcmp(em + pad_len, sha256_prefix, sizeof(sha256_prefix)))
		return -EACCES;

	if (memcmp(em + pad_len + sizeof(sha256_prefix), digest,
		   SHA256_SUM_LEN))
		return -EACCES;

	return 0;
}
