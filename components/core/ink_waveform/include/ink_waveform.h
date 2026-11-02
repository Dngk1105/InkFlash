/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Bảng LUT chế dộ B/W(docs/hardware/parts/weact-epaper-3.7/waveform.md)
 * 5 bảng, 6 group * 7 byte = 1 byte lặp group + 4 byte phase (2 bit điện áp, 6 bit số frame) + 2 byte lặp state
 * state 1 = phase 1+2, state 2 = phase 3+4
 */
#include <stdint.h>

#define INK_WAVEFORM_GROUP_COUNT 6
#define INK_WAVEFORM_GROUP_BYTES 7
#define INK_WAVEFORM_TABLE_BYTES (INK_WAVEFORM_GROUP_COUNT * INK_WAVEFORM_GROUP_BYTES)
#define INK_WAVEFORM_MAX_FRAMES  63 /* 6 bit */

/* ---- Lỗi ---- */

typedef enum : uint8_t {
    INK_WAVEFORM_OK = 0,
    INK_WAVEFORM_ERROR_ARGUMENT,
} ink_waveform_error_t;

/* ---- Type ---- */

typedef enum : uint8_t {
    INK_WAVEFORM_TABLE_VCOM,
    INK_WAVEFORM_TABLE_WHITE_TO_WHITE,
    INK_WAVEFORM_TABLE_BLACK_TO_WHITE,
    INK_WAVEFORM_TABLE_WHITE_TO_BLACK,
    INK_WAVEFORM_TABLE_BLACK_TO_BLACK,
    INK_WAVEFORM_TABLE_COUNT,
} ink_waveform_table_t;

typedef enum : uint8_t {
    INK_WAVEFORM_LEVEL_ZERO = 0,
    INK_WAVEFORM_LEVEL_VSH  = 1, /**< kéo về đen */
    INK_WAVEFORM_LEVEL_VSL  = 2, /**< kéo về trắng */
    INK_WAVEFORM_LEVEL_VDHR = 3,
} ink_waveform_level_t;

typedef struct {
    uint8_t tables[INK_WAVEFORM_TABLE_COUNT][INK_WAVEFORM_TABLE_BYTES];
} ink_waveform_t;

typedef struct {
    uint8_t frames;      /**< số khung kéo của mỗi lần đổi màu, 1–63 */
    uint8_t hold_frames; /**< xung giữ: số khung cuối kéo lại cùng màu cho điểm không đổi, 0–frames */
} ink_waveform_parameters_t;

/** Điện tích tính theo khung: VSH +1, VSL −1, mức khác 0. */
typedef struct {
    int32_t cycle;      /**< trắng → đen rồi đen → trắng; khác 0 là lệch */
    int32_t hold_white; /**< trắng giữ trắng */
    int32_t hold_black; /**< đen giữ đen */
} ink_waveform_charge_t;

/* ---- API ---- */

/**
 * Dựng 5 bảng một nhóm: điểm đổi màu kéo `frames` khung, điểm giữ màu đứng yên rồi kéo lại
 * `hold_frames` khung cuối, VCOM đứng yên.
 * Gọi từ: driver màn, khi nạp profile LUT.
 * @return INK_WAVEFORM_ERROR_ARGUMENT nếu có con trỏ nullptr, frames ngoài 1–63 hoặc hold_frames lớn hơn frames
 */
[[nodiscard]] ink_waveform_error_t ink_waveform_build(const ink_waveform_parameters_t *parameters, ink_waveform_t *out_waveform);

/**
 * Số khung của một bảng, đã tính lặp.
 * @return 0 nếu nullptr
 */
uint32_t ink_waveform_table_frames(const uint8_t table[INK_WAVEFORM_TABLE_BYTES]);

/**
 * Số khung của cả bộ: bảng dài nhất (controller cấp 0 V cho phần còn lại của bảng ngắn).
 * @return 0 nếu nullptr
 */
uint32_t ink_waveform_frames(const ink_waveform_t *waveform);

/**
 * Tính điện tích của các bảng nguồn.
 * @return INK_WAVEFORM_ERROR_ARGUMENT nếu có con trỏ nullptr
 */
[[nodiscard]] ink_waveform_error_t ink_waveform_charge(const ink_waveform_t *waveform, ink_waveform_charge_t *out_charge);
