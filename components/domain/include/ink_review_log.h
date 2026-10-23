#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * ReviewLog: mỗi lần chấm điểm một thẻ ghi thêm MỘT bản ghi cố định 16 byte vào cuối file, không bao giờ sửa
 * bản ghi cũ. Mỗi bản ghi có CRC riêng: rút điện giữa lúc ghi chỉ làm hỏng bản ghi cuối, đọc lại sẽ bỏ qua nó.
 * Lịch ôn (FSRS) luôn tính lại được từ chuỗi bản ghi này.
 *
 * Định dạng trên flash (little-endian):
 *   [0] magic 0xA5  [1] version  [2] grade (1..4)  [3] flags
 *   [4..7] card_id  [8..11] time_s (giây Unix)  [12..15] crc32 của byte 0..11
 */
#define INK_REVIEW_RECORD_SIZE 16
#define INK_REVIEW_MAGIC 0xA5
#define INK_REVIEW_VERSION 1

typedef enum {
    INK_GRADE_AGAIN = 1,
    INK_GRADE_HARD = 2,
    INK_GRADE_GOOD = 3,
    INK_GRADE_EASY = 4,
} ink_grade_t;

typedef struct {
    uint32_t card_id;
    uint32_t time_s;
    uint8_t grade; /* ink_grade_t */
    uint8_t flags;
} ink_review_t;

typedef enum {
    INK_REVIEW_OK = 0,
    INK_REVIEW_BAD_MAGIC,
    INK_REVIEW_BAD_VERSION,
    INK_REVIEW_BAD_CRC,
    INK_REVIEW_BAD_GRADE,
} ink_review_status_t;

void ink_review_encode(const ink_review_t *in, uint8_t out[INK_REVIEW_RECORD_SIZE]);
ink_review_status_t ink_review_decode(const uint8_t in[INK_REVIEW_RECORD_SIZE], ink_review_t *out);
