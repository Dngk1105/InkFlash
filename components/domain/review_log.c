#include "ink_crc32.h"
#include "ink_review_log.h"

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void ink_review_encode(const ink_review_t *in, uint8_t out[INK_REVIEW_RECORD_SIZE])
{
    out[0] = INK_REVIEW_MAGIC;
    out[1] = INK_REVIEW_VERSION;
    out[2] = in->grade;
    out[3] = in->flags;
    put_u32(out + 4, in->card_id);
    put_u32(out + 8, in->time_s);
    put_u32(out + 12, ink_crc32(out, 12));
}

ink_review_status_t ink_review_decode(const uint8_t in[INK_REVIEW_RECORD_SIZE], ink_review_t *out)
{
    if (in[0] != INK_REVIEW_MAGIC) {
        return INK_REVIEW_BAD_MAGIC;
    }
    if (in[1] != INK_REVIEW_VERSION) {
        return INK_REVIEW_BAD_VERSION;
    }
    if (get_u32(in + 12) != ink_crc32(in, 12)) {
        return INK_REVIEW_BAD_CRC;
    }
    if (in[2] < INK_GRADE_AGAIN || in[2] > INK_GRADE_EASY) {
        return INK_REVIEW_BAD_GRADE;
    }
    out->grade = in[2];
    out->flags = in[3];
    out->card_id = get_u32(in + 4);
    out->time_s = get_u32(in + 8);
    return INK_REVIEW_OK;
}
