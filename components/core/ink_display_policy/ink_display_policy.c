/* Thiết kế: docs/design/display/display.md */
#include "ink_display_policy.h"

#include <stddef.h>

static bool config_valid(const ink_display_policy_config_t *config) {
    return config->width != 0
        && config->height != 0
        && config->x_align != 0
        && config->partials_before_full != 0
        && config->max_partial_percent != 0
        && config->max_partial_percent <= 100
        && config->edge_margin <= INK_DISPLAY_POLICY_MAX_EDGE_MARGIN;
}

ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t *state, const ink_display_policy_config_t *config) {
    if (state == nullptr || config == nullptr || !config_valid(config)) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    state->priv.config        = *config;
    state->priv.partial_count = 0;
    state->priv.shown_valid   = false;
    state->priv.after_wake    = false;
    return INK_DISPLAY_POLICY_OK;
}

void ink_display_policy_invalidate(ink_display_policy_state_t *state) {
    if (state == nullptr) { return; }
    state->priv.shown_valid = false;
    state->priv.after_wake  = false;
}

void ink_display_policy_assume_shown(ink_display_policy_state_t *state) {
    if (state == nullptr) { return; }
    state->priv.shown_valid = true;
    state->priv.after_wake  = true;
}

ink_display_policy_error_t ink_display_policy_set_edge_margin(ink_display_policy_state_t *state, uint8_t margin) {
    if (state == nullptr || margin > INK_DISPLAY_POLICY_MAX_EDGE_MARGIN) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    state->priv.config.edge_margin = margin;
    return INK_DISPLAY_POLICY_OK;
}

/* ---- So khung ---- */

static size_t row_bytes(const ink_display_policy_config_t *config) {
    return ((size_t)config->width + 7) / 8;
}

/* Byte khác nhau giữa hai khung; bỏ bit đệm cuối hàng. */
static uint8_t diff_byte(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next, uint32_t y, size_t i) {
    const size_t offset = y * row_bytes(config) + i;
    uint8_t      diff   = shown[offset] ^ next[offset];
    if (i == row_bytes(config) - 1 && config->width % 8 != 0) { diff &= (uint8_t)(0xFF << (8 - config->width % 8)); }
    return diff;
}

/* Bit của điểm thứ `pixel` (0..7) trong byte; điểm trái nhất ở bit cao nhất. */
static uint8_t pixel_bit(uint32_t pixel) {
    return (uint8_t)(0x80U >> pixel);
}

typedef struct {
    uint32_t left; /* các mép đều tính cả điểm ở mép */
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
} bounds_t;

/* Khung bao các điểm khác nhau; false nếu hai khung giống hệt. */
static bool find_changed_bounds(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next,
                                bounds_t *out_bounds) {
    bounds_t bounds = {.left = UINT32_MAX, .right = 0, .top = UINT32_MAX, .bottom = 0};
    for (uint32_t y = 0; y < config->height; y++) {
        for (size_t i = 0; i < row_bytes(config); i++) {
            const uint8_t diff = diff_byte(config, shown, next, y, i);
            if (diff == 0) { continue; }
            for (uint32_t pixel = 0; pixel < 8; pixel++) {
                if ((diff & pixel_bit(pixel)) == 0) { continue; }
                const uint32_t x = (uint32_t)i * 8 + pixel;
                if (x < bounds.left) { bounds.left = x; }
                if (x > bounds.right) { bounds.right = x; }
            }
            if (y < bounds.top) { bounds.top = y; }
            bounds.bottom = y;
        }
    }
    if (bounds.top == UINT32_MAX) { return false; }
    *out_bounds = bounds;
    return true;
}

/* Nới khung thêm `margin` điểm, cắt theo màn, căn mép x theo x_align. */
static ink_display_policy_rect_t make_window(const ink_display_policy_config_t *config, bounds_t bounds, uint32_t margin) {
    const uint32_t left   = bounds.left > margin ? bounds.left - margin : 0;
    const uint32_t top    = bounds.top > margin ? bounds.top - margin : 0;
    uint32_t       right  = bounds.right + margin + 1; /* mép phải, không gồm */
    uint32_t       bottom = bounds.bottom + margin + 1;
    if (bottom > config->height) { bottom = config->height; }

    const uint32_t align        = config->x_align;
    const uint32_t aligned_left = left / align * align;
    right                       = (right + align - 1) / align * align;
    if (right > config->width) { right = config->width; }
    return (ink_display_policy_rect_t){
        .x      = (uint16_t)aligned_left,
        .y      = (uint16_t)top,
        .width  = (uint16_t)(right - aligned_left),
        .height = (uint16_t)(bottom - top),
    };
}

/* ---- Chọn cách vẽ ---- */

static void decide_full(ink_display_policy_state_t *state, ink_display_policy_decision_t *out_decision) {
    state->priv.partial_count = 0;
    state->priv.shown_valid   = true;

    *out_decision = (ink_display_policy_decision_t){
        .mode   = INK_DISPLAY_POLICY_MODE_FULL,
        .window = {.x = 0, .y = 0, .width = state->priv.config.width, .height = state->priv.config.height},
    };
}

ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next,
                                                     ink_display_policy_decision_t *out_decision) {
    if (state == nullptr || next == nullptr || out_decision == nullptr) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    if (state->priv.shown_valid && shown == nullptr) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    const ink_display_policy_config_t *config = &state->priv.config;

    if (!state->priv.shown_valid) {
        decide_full(state, out_decision);
        return INK_DISPLAY_POLICY_OK;
    }
    bounds_t changed;
    if (!find_changed_bounds(config, shown, next, &changed)) {
        *out_decision = (ink_display_policy_decision_t){.mode = INK_DISPLAY_POLICY_MODE_NONE};
        return INK_DISPLAY_POLICY_OK;
    }
    if (state->priv.after_wake) {
        state->priv.after_wake = false;
        state->priv.partial_count++;
        *out_decision = (ink_display_policy_decision_t){
            .mode   = INK_DISPLAY_POLICY_MODE_PARTIAL,
            .window = {.x = 0, .y = 0, .width = config->width, .height = config->height},
        };
        return INK_DISPLAY_POLICY_OK;
    }
    const ink_display_policy_rect_t window       = make_window(config, changed, config->edge_margin);
    const uint64_t                  changed_area = (uint64_t)window.width * window.height;
    const uint64_t                  screen_area  = (uint64_t)config->width * config->height;
    if (state->priv.partial_count >= config->partials_before_full || changed_area * 100 > screen_area * config->max_partial_percent) {
        decide_full(state, out_decision);
        return INK_DISPLAY_POLICY_OK;
    }
    state->priv.partial_count++;
    *out_decision = (ink_display_policy_decision_t){.mode = INK_DISPLAY_POLICY_MODE_PARTIAL, .window = window};
    return INK_DISPLAY_POLICY_OK;
}

/* ---- Đẩy lại viền ---- */

/* Nới các điểm đổi của byte `i` hàng `y` sang ngang `margin` điểm, gồm cả điểm
 * ở hai byte bên cạnh. */
static uint8_t spread_horizontally(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next, uint32_t y,
                                   size_t i, uint32_t margin) {
    const uint8_t diff   = diff_byte(config, shown, next, y, i);
    const uint8_t left   = i > 0 ? diff_byte(config, shown, next, y, i - 1) : 0;
    const uint8_t right  = i + 1 < row_bytes(config) ? diff_byte(config, shown, next, y, i + 1) : 0;
    uint8_t       spread = diff;
    for (uint32_t shift = 1; shift <= margin; shift++) {
        /* điểm x + shift về vị trí x, và điểm x - shift về vị trí x */
        spread |= (uint8_t)(diff << shift) | (uint8_t)(right >> (8 - shift));
        spread |= (uint8_t)(diff >> shift) | (uint8_t)(left << (8 - shift));
    }
    return spread;
}

ink_display_policy_error_t ink_display_policy_edge_plane(const ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next,
                                                         ink_display_policy_rect_t window, uint8_t *out_old) {
    if (state == nullptr || shown == nullptr || next == nullptr || out_old == nullptr) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    const ink_display_policy_config_t *config = &state->priv.config;
    if (window.width == 0
        || window.height == 0
        || (uint32_t)window.x + window.width > config->width
        || (uint32_t)window.y + window.height > config->height) {
        return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
    }
    const uint32_t margin     = config->edge_margin;
    const size_t   first_byte = window.x / 8;
    const size_t   last_byte  = ((size_t)window.x + window.width - 1) / 8;
    for (uint32_t y = window.y; y < (uint32_t)window.y + window.height; y++) {
        const uint32_t top    = y > margin ? y - margin : 0;
        const uint32_t bottom = y + margin < config->height ? y + margin : config->height - 1u;
        for (size_t i = first_byte; i <= last_byte; i++) {
            uint8_t near_change = 0;
            for (uint32_t row = top; row <= bottom; row++) { near_change |= spread_horizontally(config, shown, next, row, i, margin); }
            const uint8_t halo                 = (uint8_t)(near_change & ~diff_byte(config, shown, next, y, i));
            out_old[y * row_bytes(config) + i] = (uint8_t)(shown[y * row_bytes(config) + i] ^ halo);
        }
    }
    return INK_DISPLAY_POLICY_OK;
}
