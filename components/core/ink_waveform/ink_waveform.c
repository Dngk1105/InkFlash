/* Thiết kế: docs/design/display/display.md */
#include "ink_waveform.h"

#include <stddef.h>
#include <string.h>

/* Vị trí byte trong một nhóm. */
#define GROUP_REPEAT   0
#define PHASE_FIRST    1
#define PHASE_COUNT    4
#define STATE_1_REPEAT 5
#define STATE_2_REPEAT 6

static uint8_t phase(ink_waveform_level_t level, uint32_t frames) {
    return (uint8_t)((level << 6) | (frames & 0x3F));
}

static uint32_t repeat_count(uint8_t value) {
    return value == 0 ? 1U : value;
}

/* Một nhóm, hai pha đầu của trạng thái 1; các nhóm còn lại bằng 0. */
static void set_table(uint8_t *table, ink_waveform_level_t first_level, uint32_t first_frames, ink_waveform_level_t second_level,
                      uint32_t second_frames) {
    memset(table, 0, INK_WAVEFORM_TABLE_BYTES);
    table[GROUP_REPEAT]    = 1;
    table[PHASE_FIRST]     = phase(first_level, first_frames);
    table[PHASE_FIRST + 1] = phase(second_level, second_frames);
    table[STATE_1_REPEAT]  = 1;
    table[STATE_2_REPEAT]  = 1;
}

ink_waveform_error_t ink_waveform_build(const ink_waveform_parameters_t *parameters, ink_waveform_t *out_waveform) {
    if (parameters == nullptr || out_waveform == nullptr) { return INK_WAVEFORM_ERROR_ARGUMENT; }
    const uint32_t frames = parameters->frames;
    const uint32_t hold   = parameters->hold_frames;
    if (frames == 0 || frames > INK_WAVEFORM_MAX_FRAMES || hold > frames) { return INK_WAVEFORM_ERROR_ARGUMENT; }
    uint8_t (*tables)[INK_WAVEFORM_TABLE_BYTES] = out_waveform->tables;
    /* Mức 00 của bảng VCOM là VCOM_DC: VCOM đứng yên. */
    set_table(tables[INK_WAVEFORM_TABLE_VCOM], INK_WAVEFORM_LEVEL_ZERO, frames, INK_WAVEFORM_LEVEL_ZERO, 0);
    set_table(tables[INK_WAVEFORM_TABLE_BLACK_TO_WHITE], INK_WAVEFORM_LEVEL_VSL, frames, INK_WAVEFORM_LEVEL_ZERO, 0);
    set_table(tables[INK_WAVEFORM_TABLE_WHITE_TO_BLACK], INK_WAVEFORM_LEVEL_VSH, frames, INK_WAVEFORM_LEVEL_ZERO, 0);
    set_table(tables[INK_WAVEFORM_TABLE_WHITE_TO_WHITE], INK_WAVEFORM_LEVEL_ZERO, frames - hold, INK_WAVEFORM_LEVEL_VSL, hold);
    set_table(tables[INK_WAVEFORM_TABLE_BLACK_TO_BLACK], INK_WAVEFORM_LEVEL_ZERO, frames - hold, INK_WAVEFORM_LEVEL_VSH, hold);
    return INK_WAVEFORM_OK;
}

/* ---- Đếm ---- */

/* Gọi `visit` cho từng pha, kèm số khung đã nhân lặp. */
typedef void (*phase_visitor_t)(uint8_t phase_byte, uint32_t frames, void *user_data);

static void for_each_phase(const uint8_t *table, phase_visitor_t visit, void *user_data) {
    for (size_t group = 0; group < INK_WAVEFORM_GROUP_COUNT; group++) {
        const uint8_t *bytes        = &table[group * INK_WAVEFORM_GROUP_BYTES];
        const uint32_t group_repeat = repeat_count(bytes[GROUP_REPEAT]);
        for (size_t i = 0; i < PHASE_COUNT; i++) {
            const uint8_t  phase_byte   = bytes[PHASE_FIRST + i];
            const uint32_t state_repeat = repeat_count(bytes[i < 2 ? STATE_1_REPEAT : STATE_2_REPEAT]);
            visit(phase_byte, (phase_byte & 0x3FU) * state_repeat * group_repeat, user_data);
        }
    }
}

static void add_frames(uint8_t phase_byte, uint32_t frames, void *user_data) {
    (void)phase_byte;
    *(uint32_t *)user_data += frames;
}

static void add_charge(uint8_t phase_byte, uint32_t frames, void *user_data) {
    int32_t                   *charge = user_data;
    const ink_waveform_level_t level  = (ink_waveform_level_t)(phase_byte >> 6);
    if (level == INK_WAVEFORM_LEVEL_VSH) { *charge += (int32_t)frames; }
    if (level == INK_WAVEFORM_LEVEL_VSL) { *charge -= (int32_t)frames; }
}

uint32_t ink_waveform_table_frames(const uint8_t table[INK_WAVEFORM_TABLE_BYTES]) {
    uint32_t total = 0;
    if (table != nullptr) { for_each_phase(table, add_frames, &total); }
    return total;
}

uint32_t ink_waveform_frames(const ink_waveform_t *waveform) {
    if (waveform == nullptr) { return 0; }
    uint32_t longest = 0;
    for (size_t i = 0; i < INK_WAVEFORM_TABLE_COUNT; i++) {
        const uint32_t frames = ink_waveform_table_frames(waveform->tables[i]);
        if (frames > longest) { longest = frames; }
    }
    return longest;
}

static int32_t table_charge(const uint8_t *table) {
    int32_t charge = 0;
    for_each_phase(table, add_charge, &charge);
    return charge;
}

ink_waveform_error_t ink_waveform_charge(const ink_waveform_t *waveform, ink_waveform_charge_t *out_charge) {
    if (waveform == nullptr || out_charge == nullptr) { return INK_WAVEFORM_ERROR_ARGUMENT; }
    const uint8_t (*tables)[INK_WAVEFORM_TABLE_BYTES] = waveform->tables;

    *out_charge = (ink_waveform_charge_t){
        .cycle      = table_charge(tables[INK_WAVEFORM_TABLE_WHITE_TO_BLACK]) + table_charge(tables[INK_WAVEFORM_TABLE_BLACK_TO_WHITE]),
        .hold_white = table_charge(tables[INK_WAVEFORM_TABLE_WHITE_TO_WHITE]),
        .hold_black = table_charge(tables[INK_WAVEFORM_TABLE_BLACK_TO_BLACK]),
    };
    return INK_WAVEFORM_OK;
}
