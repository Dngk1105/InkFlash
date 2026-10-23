#include <string.h>
#include "ink_crc32.h"
#include "ink_review_log.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* Giá trị chuẩn của CRC-32/IEEE cho chuỗi "123456789" (giống zlib, Python binascii.crc32). */
static void test_crc32_check_value(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926, ink_crc32("123456789", 9));
}

static void test_crc32_incremental_matches_one_shot(void)
{
    uint32_t crc = ink_crc32_update(0, "12345", 5);
    crc = ink_crc32_update(crc, "6789", 4);
    TEST_ASSERT_EQUAL_HEX32(ink_crc32("123456789", 9), crc);
}

static void test_review_roundtrip(void)
{
    const ink_review_t in = { .card_id = 0x12345678, .time_s = 1792800000, .grade = INK_GRADE_GOOD, .flags = 2 };
    uint8_t buf[INK_REVIEW_RECORD_SIZE];
    ink_review_encode(&in, buf);
    ink_review_t out;
    TEST_ASSERT_EQUAL(INK_REVIEW_OK, ink_review_decode(buf, &out));
    TEST_ASSERT_EQUAL_UINT32(in.card_id, out.card_id);
    TEST_ASSERT_EQUAL_UINT32(in.time_s, out.time_s);
    TEST_ASSERT_EQUAL_UINT8(in.grade, out.grade);
    TEST_ASSERT_EQUAL_UINT8(in.flags, out.flags);
}

/* Rút điện giữa lúc ghi: mọi byte hỏng trong bản ghi đều phải bị phát hiện. */
static void test_review_detects_any_flipped_byte(void)
{
    const ink_review_t in = { .card_id = 7, .time_s = 100, .grade = INK_GRADE_HARD };
    uint8_t good[INK_REVIEW_RECORD_SIZE];
    ink_review_encode(&in, good);
    for (int i = 0; i < INK_REVIEW_RECORD_SIZE; i++) {
        uint8_t bad[INK_REVIEW_RECORD_SIZE];
        memcpy(bad, good, sizeof(bad));
        bad[i] ^= 0x01;
        ink_review_t out;
        TEST_ASSERT_NOT_EQUAL_MESSAGE(INK_REVIEW_OK, ink_review_decode(bad, &out), "byte hong khong bi phat hien");
    }
}

static void test_review_rejects_erased_flash(void)
{
    uint8_t erased[INK_REVIEW_RECORD_SIZE];
    memset(erased, 0xFF, sizeof(erased)); /* flash vừa xoá */
    ink_review_t out;
    TEST_ASSERT_EQUAL(INK_REVIEW_BAD_MAGIC, ink_review_decode(erased, &out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc32_check_value);
    RUN_TEST(test_crc32_incremental_matches_one_shot);
    RUN_TEST(test_review_roundtrip);
    RUN_TEST(test_review_detects_any_flipped_byte);
    RUN_TEST(test_review_rejects_erased_flash);
    return UNITY_END();
}
