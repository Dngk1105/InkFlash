/* Thiết kế: docs/design/display-pipeline.md */
#include "ink_display_policy.h"

#include <stddef.h>

ink_display_policy_error_t
ink_display_policy_init(ink_display_policy_state_t *state,
                        const ink_display_policy_config_t *config) {
  if (state == nullptr || config == nullptr) {
    return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
  }
  if (config->width == 0 || config->height == 0 || config->x_align == 0 ||
      config->partials_before_full == 0 || config->max_partial_percent == 0 ||
      config->max_partial_percent > 100) {
    return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
  }
  state->priv.config = *config;
  state->priv.partial_count = 0;
  state->priv.shown_valid = false;
  return INK_DISPLAY_POLICY_OK;
}

void ink_display_policy_invalidate(ink_display_policy_state_t *state) {
  if (state == nullptr) {
    return;
  }
  state->priv.shown_valid = false;
}

/* ---- So khung ---- */

/* Bit của điểm thứ `pixel` (0..7) trong byte; điểm trái nhất ở bit cao nhất. */
static uint8_t pixel_bit(uint32_t pixel) { return (uint8_t)(0x80U >> pixel); }

/* Hình chữ nhật bao các điểm khác nhau; false nếu hai khung giống hệt. Bỏ qua
 * bit đệm cuối hàng. */
static bool find_changed_rect(const ink_display_policy_config_t *config,
                              const uint8_t *shown, const uint8_t *next,
                              ink_display_policy_rect_t *out_rect) {
  const size_t row_bytes = ((size_t)config->width + 7) / 8;
  const uint32_t tail_pixels = config->width % 8;
  const uint8_t tail_mask =
      tail_pixels == 0 ? 0xFF : (uint8_t)(0xFF << (8 - tail_pixels));

  uint32_t left = UINT32_MAX, right = 0, top = UINT32_MAX, bottom = 0;
  for (uint32_t y = 0; y < config->height; y++) {
    const uint8_t *shown_row = shown + y * row_bytes;
    const uint8_t *next_row = next + y * row_bytes;
    for (size_t i = 0; i < row_bytes; i++) {
      uint8_t diff = shown_row[i] ^ next_row[i];
      if (i == row_bytes - 1) {
        diff &= tail_mask;
      }
      if (diff == 0) {
        continue;
      }
      for (uint32_t pixel = 0; pixel < 8; pixel++) {
        if ((diff & pixel_bit(pixel)) == 0) {
          continue;
        }
        const uint32_t x = (uint32_t)i * 8 + pixel;
        if (x < left) {
          left = x;
        }
        if (x > right) {
          right = x;
        }
      }
      if (y < top) {
        top = y;
      }
      bottom = y;
    }
  }
  if (top == UINT32_MAX) {
    return false;
  }

  const uint32_t align = config->x_align;
  const uint32_t aligned_left = left / align * align;
  uint32_t aligned_right =
      (right / align + 1) * align; /* mép phải, không gồm */
  if (aligned_right > config->width) {
    aligned_right = config->width;
  }
  *out_rect = (ink_display_policy_rect_t){
      .x = (uint16_t)aligned_left,
      .y = (uint16_t)top,
      .width = (uint16_t)(aligned_right - aligned_left),
      .height = (uint16_t)(bottom - top + 1),
  };
  return true;
}

/* ---- Chọn cách vẽ ---- */

static void decide_full(ink_display_policy_state_t *state,
                        ink_display_policy_decision_t *out_decision) {
  state->priv.partial_count = 0;
  state->priv.shown_valid = true;
  *out_decision = (ink_display_policy_decision_t){
      .mode = INK_DISPLAY_POLICY_MODE_FULL,
      .window = {.x = 0,
                 .y = 0,
                 .width = state->priv.config.width,
                 .height = state->priv.config.height},
  };
}

ink_display_policy_error_t
ink_display_policy_decide(ink_display_policy_state_t *state,
                          const uint8_t *shown, const uint8_t *next,
                          ink_display_policy_decision_t *out_decision) {
  if (state == nullptr || next == nullptr || out_decision == nullptr) {
    return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
  }
  if (state->priv.shown_valid && shown == nullptr) {
    return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
  }
  const ink_display_policy_config_t *config = &state->priv.config;

  if (!state->priv.shown_valid) {
    decide_full(state, out_decision);
    return INK_DISPLAY_POLICY_OK;
  }
  ink_display_policy_rect_t changed;
  if (!find_changed_rect(config, shown, next, &changed)) {
    *out_decision =
        (ink_display_policy_decision_t){.mode = INK_DISPLAY_POLICY_MODE_NONE};
    return INK_DISPLAY_POLICY_OK;
  }
  const uint64_t changed_area = (uint64_t)changed.width * changed.height;
  const uint64_t screen_area = (uint64_t)config->width * config->height;
  if (!config->has_partial ||
      state->priv.partial_count >= config->partials_before_full ||
      changed_area * 100 > screen_area * config->max_partial_percent) {
    decide_full(state, out_decision);
    return INK_DISPLAY_POLICY_OK;
  }
  state->priv.partial_count++;
  *out_decision = (ink_display_policy_decision_t){
      .mode = INK_DISPLAY_POLICY_MODE_PARTIAL, .window = changed};
  return INK_DISPLAY_POLICY_OK;
}
