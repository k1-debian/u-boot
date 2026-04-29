#ifndef __SECURITY_PUBKEY_H__
#define __SECURITY_PUBKEY_H__

#define SECURITY_PUBKEY_ALGO_UNKNOWN		0U
#define SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1	1U

#define SECURITY_PUBKEY_FLAG_OWNS_DATA		0x00000001U

#define SECURITY_RSA2048_BYTES		256U
#define SECURITY_RSA2048_WORDS		(SECURITY_RSA2048_BYTES / 4)

struct security_pubkey {
	unsigned int key_id;
	unsigned int algo;
	const void *data;
	unsigned int data_len;
	unsigned int flags;
};

struct security_rsa_public_key {
	unsigned int len;
	unsigned int n0inv;
	unsigned int modulus[SECURITY_RSA2048_WORDS];
	unsigned int rr[SECURITY_RSA2048_WORDS];
};

int security_pubkey_parse_rsa(const struct security_pubkey *pubkey,
			      struct security_rsa_public_key *out_key);
int security_pubkey_dup(const struct security_pubkey *src,
			struct security_pubkey *out);
void security_pubkey_release(struct security_pubkey *pubkey);
int security_pubkey_verify_rsa2048_sha256(
	const struct security_pubkey *pubkey,
	const unsigned char *sig, unsigned int sig_len,
	const unsigned char *digest, unsigned int digest_len);

#endif
