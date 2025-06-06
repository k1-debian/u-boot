#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include <h264_common.h>

#include "intreadwrite.h"
#include "mem.h"

#include "bytestream.h"
#include "h264.h"
#include "h2645_parse.h"

int ff_h2645_extract_rbsp(const uint8_t *src, int length,
                	H2645NAL *nal, int small_padding)
{
    int i;
    int consumed = length;

    nal->skipped_bytes = 0;
#define STARTCODE_TEST                                                  \
        if (i + 2 < length && src[i + 1] == 0 && src[i + 2] < 3) {     \
            if (src[i + 2] != 3 && src[i + 2] != 0) {                   \
                /* startcode, so we must be past the end */             \
                length = i;                                             \
            }                                                           \
            break;                                                      \
        }

    for (i = 0; i + 1 < length; i += 2) {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
    consumed = i;
    nal->data = nal->raw_data = src;
    nal->size = nal->raw_size = consumed + 1;
    return consumed;
}

static const char *hevc_nal_type_name[64] = {
    "TRAIL_N", // HEVC_NAL_TRAIL_N
    "TRAIL_R", // HEVC_NAL_TRAIL_R
    "TSA_N", // HEVC_NAL_TSA_N
    "TSA_R", // HEVC_NAL_TSA_R
    "STSA_N", // HEVC_NAL_STSA_N
    "STSA_R", // HEVC_NAL_STSA_R
    "RADL_N", // HEVC_NAL_RADL_N
    "RADL_R", // HEVC_NAL_RADL_R
    "RASL_N", // HEVC_NAL_RASL_N
    "RASL_R", // HEVC_NAL_RASL_R
    "RSV_VCL_N10", // HEVC_NAL_VCL_N10
    "RSV_VCL_R11", // HEVC_NAL_VCL_R11
    "RSV_VCL_N12", // HEVC_NAL_VCL_N12
    "RSV_VLC_R13", // HEVC_NAL_VCL_R13
    "RSV_VCL_N14", // HEVC_NAL_VCL_N14
    "RSV_VCL_R15", // HEVC_NAL_VCL_R15
    "BLA_W_LP", // HEVC_NAL_BLA_W_LP
    "BLA_W_RADL", // HEVC_NAL_BLA_W_RADL
    "BLA_N_LP", // HEVC_NAL_BLA_N_LP
    "IDR_W_RADL", // HEVC_NAL_IDR_W_RADL
    "IDR_N_LP", // HEVC_NAL_IDR_N_LP
    "CRA_NUT", // HEVC_NAL_CRA_NUT
    "IRAP_IRAP_VCL22", // HEVC_NAL_IRAP_VCL22
    "IRAP_IRAP_VCL23", // HEVC_NAL_IRAP_VCL23
    "RSV_VCL24", // HEVC_NAL_RSV_VCL24
    "RSV_VCL25", // HEVC_NAL_RSV_VCL25
    "RSV_VCL26", // HEVC_NAL_RSV_VCL26
    "RSV_VCL27", // HEVC_NAL_RSV_VCL27
    "RSV_VCL28", // HEVC_NAL_RSV_VCL28
    "RSV_VCL29", // HEVC_NAL_RSV_VCL29
    "RSV_VCL30", // HEVC_NAL_RSV_VCL30
    "RSV_VCL31", // HEVC_NAL_RSV_VCL31
    "VPS", // HEVC_NAL_VPS
    "SPS", // HEVC_NAL_SPS
    "PPS", // HEVC_NAL_PPS
    "AUD", // HEVC_NAL_AUD
    "EOS_NUT", // HEVC_NAL_EOS_NUT
    "EOB_NUT", // HEVC_NAL_EOB_NUT
    "FD_NUT", // HEVC_NAL_FD_NUT
    "SEI_PREFIX", // HEVC_NAL_SEI_PREFIX
    "SEI_SUFFIX", // HEVC_NAL_SEI_SUFFIX
    "RSV_NVCL41", // HEVC_NAL_RSV_NVCL41
    "RSV_NVCL42", // HEVC_NAL_RSV_NVCL42
    "RSV_NVCL43", // HEVC_NAL_RSV_NVCL43
    "RSV_NVCL44", // HEVC_NAL_RSV_NVCL44
    "RSV_NVCL45", // HEVC_NAL_RSV_NVCL45
    "RSV_NVCL46", // HEVC_NAL_RSV_NVCL46
    "RSV_NVCL47", // HEVC_NAL_RSV_NVCL47
    "UNSPEC48", // HEVC_NAL_UNSPEC48
    "UNSPEC49", // HEVC_NAL_UNSPEC49
    "UNSPEC50", // HEVC_NAL_UNSPEC50
    "UNSPEC51", // HEVC_NAL_UNSPEC51
    "UNSPEC52", // HEVC_NAL_UNSPEC52
    "UNSPEC53", // HEVC_NAL_UNSPEC53
    "UNSPEC54", // HEVC_NAL_UNSPEC54
    "UNSPEC55", // HEVC_NAL_UNSPEC55
    "UNSPEC56", // HEVC_NAL_UNSPEC56
    "UNSPEC57", // HEVC_NAL_UNSPEC57
    "UNSPEC58", // HEVC_NAL_UNSPEC58
    "UNSPEC59", // HEVC_NAL_UNSPEC59
    "UNSPEC60", // HEVC_NAL_UNSPEC60
    "UNSPEC61", // HEVC_NAL_UNSPEC61
    "UNSPEC62", // HEVC_NAL_UNSPEC62
    "UNSPEC63", // HEVC_NAL_UNSPEC63
};

static const char *hevc_nal_unit_name(int nal_type)
{
    av_assert0(nal_type >= 0 && nal_type < 64);
    return hevc_nal_type_name[nal_type];
}

static const char *h264_nal_type_name[32] = {
    "Unspecified 0", //H264_NAL_UNSPECIFIED
    "Coded slice of a non-IDR picture", // H264_NAL_SLICE
    "Coded slice data partition A", // H264_NAL_DPA
    "Coded slice data partition B", // H264_NAL_DPB
    "Coded slice data partition C", // H264_NAL_DPC
    "IDR", // H264_NAL_IDR_SLICE
    "SEI", // H264_NAL_SEI
    "SPS", // H264_NAL_SPS
    "PPS", // H264_NAL_PPS
    "AUD", // H264_NAL_AUD
    "End of sequence", // H264_NAL_END_SEQUENCE
    "End of stream", // H264_NAL_END_STREAM
    "Filler data", // H264_NAL_FILLER_DATA
    "SPS extension", // H264_NAL_SPS_EXT
    "Prefix", // H264_NAL_PREFIX
    "Subset SPS", // H264_NAL_SUB_SPS
    "Depth parameter set", // H264_NAL_DPS
    "Reserved 17", // H264_NAL_RESERVED17
    "Reserved 18", // H264_NAL_RESERVED18
    "Auxiliary coded picture without partitioning", // H264_NAL_AUXILIARY_SLICE
    "Slice extension", // H264_NAL_EXTEN_SLICE
    "Slice extension for a depth view or a 3D-AVC texture view", // H264_NAL_DEPTH_EXTEN_SLICE
    "Reserved 22", // H264_NAL_RESERVED22
    "Reserved 23", // H264_NAL_RESERVED23
    "Unspecified 24", // H264_NAL_UNSPECIFIED24
    "Unspecified 25", // H264_NAL_UNSPECIFIED25
    "Unspecified 26", // H264_NAL_UNSPECIFIED26
    "Unspecified 27", // H264_NAL_UNSPECIFIED27
    "Unspecified 28", // H264_NAL_UNSPECIFIED28
    "Unspecified 29", // H264_NAL_UNSPECIFIED29
    "Unspecified 30", // H264_NAL_UNSPECIFIED30
    "Unspecified 31", // H264_NAL_UNSPECIFIED31
};

static const char *h264_nal_unit_name(int nal_type)
{
    av_assert0(nal_type >= 0 && nal_type < 32);
    return h264_nal_type_name[nal_type];
}

static int get_bit_length(H2645NAL *nal, int skip_trailing_zeros)
{
    int size = nal->size;
    int v;

    while (skip_trailing_zeros && size > 0 && nal->data[size - 1] == 0)
        size--;

    if (!size)
        return 0;

    v = nal->data[size - 1];

    if (size > INT_MAX / 8)
        return AVERROR(ERANGE);
    size *= 8;

    /* remove the stop bit and following trailing zeros,
     * or nothing for damaged bitstreams */
    if (v)
        size -= ff_ctz(v) + 1;

    return size;
}

/**
 * @return AVERROR_INVALIDDATA if the packet is not a valid NAL unit,
 * 0 if the unit should be skipped, 1 otherwise
 */
static int hevc_parse_nal_header(H2645NAL *nal, void *logctx)
{
    GetBitContext *gb = &nal->gb;
    int nuh_layer_id;

    if (get_bits1(gb) != 0)
        return AVERROR_INVALIDDATA;

    nal->type = get_bits(gb, 6);

    nuh_layer_id   = get_bits(gb, 6);
    nal->temporal_id = get_bits(gb, 3) - 1;
    if (nal->temporal_id < 0)
        return AVERROR_INVALIDDATA;

    av_log(logctx, AV_LOG_DEBUG,
           "nal_unit_type: %d(%s), nuh_layer_id: %d, temporal_id: %d\n",
           nal->type, hevc_nal_unit_name(nal->type), nuh_layer_id, nal->temporal_id);

    return nuh_layer_id == 0;
}

static int h264_parse_nal_header(H2645NAL *nal, void *logctx)
{
    GetBitContext *gb = &nal->gb;
    char *src;
    char *dst;
    unsigned int si, di;
    int removed_bytes = 0;
    unsigned int length = 0;

    if (get_bits1(gb) != 0)
        return AVERROR_INVALIDDATA;

    nal->ref_idc = get_bits(gb, 2);
    nal->type    = get_bits(gb, 5);

    if(nal->type != H264_NAL_SLICE && nal->type != H264_NAL_IDR_SLICE) {
	    si = di = 0;
	    src = dst = nal->data;
	    length = nal->size;
	    while (si + 2 < length) {
		    // remove escapes (very rare 1:2^22)
		    if (src[si + 2] > 3) {
			    dst[di++] = src[si++];
			    dst[di++] = src[si++];
		    } else if (src[si] == 0 && src[si + 1] == 0 && src[si + 2] == 3) {
				    dst[di++] = 0;
				    dst[di++] = 0;
				    si       += 3;
		    }
		    dst[di++] = src[si++];
	    }

	    while (si < length)
		    dst[di++] = src[si++];

        removed_bytes = si - di;

        nal->size -= removed_bytes;

    }

    av_log(logctx, AV_LOG_DEBUG,
           "nal_unit_type: %d(%s), nal_ref_idc: %d\n",
           nal->type, h264_nal_unit_name(nal->type), nal->ref_idc);

    return 1;
}

static int find_next_start_code(const uint8_t *buf, const uint8_t *next_avc)
{
    int i = 0;

    if (buf + 3 >= next_avc)
        return next_avc - buf;

    while (buf + i + 3 < next_avc) {
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1)
            break;
        i++;
    }
    return i + 3;
}

extern void hexdump(unsigned char *buf, int len);

int ff_h2645_packet_split(H2645Packet *pkt, const uint8_t *buf, int length,
                          void *logctx, int is_nalff, int nal_length_size,
                          int small_padding)
{
    GetByteContext bc;
    int consumed, ret = 0;
    int next_avc = is_nalff ? 0 : length; //如果nal时全FF, 说明没有下一个avc， 否则下一个avc就是buffer的长度??
    int64_t padding = small_padding ? 0 : MAX_MBPAIR_SIZE;

    /* buffer是输入的原始码流.*/
    bytestream2_init(&bc, buf, length);


    pkt->nb_nals = 0;
    while (bytestream2_get_bytes_left(&bc) >= 4) {
        H2645NAL *nal;
        int extract_length = 0;
        int skip_trailing_zeros = 1;

	int buf_index;

	if (bytestream2_tell(&bc) > next_avc)
		av_log(logctx, AV_LOG_WARNING, "Exceeded next NALFF position, re-syncing.\n");

	buf_index = find_next_start_code(bc.buffer, buf + next_avc);

	bytestream2_skip(&bc, buf_index);

	if (!bytestream2_get_bytes_left(&bc)) {
		if (pkt->nb_nals > 0) {
			// No more start codes: we discarded some irrelevant
			// bytes at the end of the packet.
			return 0;
		} else {
			av_log(logctx, AV_LOG_ERROR, "No start code is found.\n");
			return AVERROR_INVALIDDATA;
		}
	}

	extract_length = FFMIN(bytestream2_get_bytes_left(&bc), next_avc - bytestream2_tell(&bc));

	if (bytestream2_tell(&bc) >= next_avc) {
		/* skip to the start of the next NAL */
		bytestream2_skip(&bc, next_avc - bytestream2_tell(&bc));
		continue;
	}

	if (pkt->nals_allocated < pkt->nb_nals + 1) {
		int new_size = pkt->nals_allocated + 1;
		void *tmp = av_realloc_array(pkt->nals, new_size, sizeof(*pkt->nals));

		if (!tmp)
			return AVERROR(ENOMEM);

		if(new_size > MAX_NALS_PER_PACKET) {
			av_log(logctx, AV_LOG_WARNING,
					"Too Many nals (%d) in one packet, max is (%d)\n",
					new_size, MAX_NALS_PER_PACKET);
			return AVERROR(ENOMEM);
		}

		pkt->nals = tmp;
		memset(pkt->nals + pkt->nals_allocated, 0,
				(new_size - pkt->nals_allocated) * sizeof(*pkt->nals));

		nal = &pkt->nals[pkt->nb_nals];
#if 0
		nal->skipped_bytes_pos_size = 1024; // initial buffer size
		nal->skipped_bytes_pos = av_malloc_array(nal->skipped_bytes_pos_size, sizeof(*nal->skipped_bytes_pos));
		if (!nal->skipped_bytes_pos)
			return AVERROR(ENOMEM);
#endif
		pkt->nals_allocated = new_size;
	}
	nal = &pkt->nals[pkt->nb_nals];

	/*提取 rbsp, 并且去掉 0x000003 序列中的0x03. */
	consumed = ff_h2645_extract_rbsp(bc.buffer, extract_length, nal, small_padding);
	if (consumed < 0)
		return consumed;

	if (is_nalff && (extract_length != consumed) && extract_length)
		av_log(logctx, AV_LOG_DEBUG,
				"NALFF: Consumed only %d bytes instead of %d\n",
				consumed, extract_length);

#if 0
	printk("----nal->data:%x, nal->size:%d\n", nal->data, nal->size);
	hexdump(nal->data, 32);
#endif

	/* 解析出来的nal递增....*/

	pkt->nb_nals++;

	bytestream2_skip(&bc, consumed);

	nal->size_bits = get_bit_length(nal, skip_trailing_zeros);

	ret = init_get_bits(&nal->gb, nal->data, nal->size_bits);
	if (ret < 0)
		return ret;

	ret = h264_parse_nal_header(nal, logctx);

	if (ret <= 0 || nal->size <= 0 || nal->size_bits <= 0) {
		if (ret < 0) {
			av_log(logctx, AV_LOG_ERROR, "Invalid NAL unit %d, skipping.\n",
					nal->type);
		}
		pkt->nb_nals--;
	}
    }

    return 0;
}


void ff_h2645_packet_uninit(H2645Packet *pkt)
{
    int i;
#if 0
    for (i = 0; i < pkt->nals_allocated; i++) {
        av_freep(&pkt->nals[i].skipped_bytes_pos);
    }
#endif
    av_freep(&pkt->nals);
    pkt->nals_allocated = 0;
}
