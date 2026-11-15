/* Design: docs/design/ui.md */
#include "ink_ui.h"

/* ---- Leaf functions ---- */

/* Length of `rect` along `axis`: width for ROW, height for COLUMN; never below 0. */
static int16_t axis_length(ink_ui_rect_t rect, ink_ui_axis_t axis) {
    const int16_t length = axis == INK_UI_AXIS_ROW ? rect.width : rect.height;
    return length > 0 ? length : 0;
}

/* Space left for flex slots: total - gap × (count - 1) - sum of fixed parts; never below 0. */
static int16_t free_length(int16_t total, int16_t gap, const ink_ui_length_t *lengths, size_t count) {
    if (total <= 0) { return 0; }
    int32_t fixed_sum = 0;
    for (size_t i = 0; i < count; i++) { fixed_sum += lengths[i].fixed; }
    const int32_t gaps = count > 1 ? (int32_t)gap * (int32_t)(count - 1) : 0;
    const int32_t free = (int32_t)total - gaps - fixed_sum;
    return free > 0 ? (int16_t)free : 0;
}

/* Sum of the flex weights. */
static uint16_t flex_sum(const ink_ui_length_t *lengths, size_t count) {
    uint16_t sum = 0;
    for (size_t i = 0; i < count; i++) { sum = (uint16_t)(sum + lengths[i].flex); }
    return sum;
}

/* Index of the last slot with flex > 0; `count` when there is none. */
static size_t last_flex_slot(const ink_ui_length_t *lengths, size_t count) {
    for (size_t i = count; i > 0; i--) {
        if (lengths[i - 1].flex > 0) { return i - 1; }
    }
    return count;
}

/* One slot of `rect`: starts `offset` px from the start of the axis, `length` px long, cut so that it stays inside
 * `rect` (length 0 when offset is past the end); spans the whole rect across the axis. */
static ink_ui_rect_t slot_rect(ink_ui_rect_t rect, ink_ui_axis_t axis, int16_t offset, int16_t length) {
    const int16_t total          = axis_length(rect, axis);
    int16_t       clamped_length = 0;
    if (offset < 0) {
        clamped_length = (int16_t)(offset + length);
        if (clamped_length > total) { clamped_length = total; }
        if (clamped_length < 0) { clamped_length = 0; }
        offset = 0;
    } else if (offset >= total) {
        offset         = total;
        clamped_length = 0;
    } else {
        clamped_length = (int16_t)(offset + length <= total ? length : total - offset);
        if (clamped_length < 0) { clamped_length = 0; }
    }
    if (axis == INK_UI_AXIS_ROW) {
        return (ink_ui_rect_t){
            .x      = (int16_t)(rect.x + offset),
            .y      = rect.y,
            .width  = clamped_length,
            .height = rect.height > 0 ? rect.height : 0,
        };
    }
    return (ink_ui_rect_t){
        .x      = rect.x,
        .y      = (int16_t)(rect.y + offset),
        .width  = rect.width > 0 ? rect.width : 0,
        .height = clamped_length,
    };
}

/* Offset of a box of `size` px inside `outer` px for one alignment: 0, (outer - size) / 2, or outer - size. */
static int16_t aligned_offset(int16_t outer, int16_t size, ink_ui_align_t align) {
    switch (align) {
        case INK_UI_ALIGN_START:
            return 0;
        case INK_UI_ALIGN_CENTER:
            return (int16_t)((outer - size) / 2);
        case INK_UI_ALIGN_END:
            return (int16_t)(outer - size);
        default:
            return 0;
    }
}

/* ---- API ---- */

ink_ui_rect_t ink_ui_rect_inset(ink_ui_rect_t rect, int16_t horizontal, int16_t vertical) {
    const int16_t width  = (int16_t)(rect.width - 2 * horizontal);
    const int16_t height = (int16_t)(rect.height - 2 * vertical);
    return (ink_ui_rect_t){
        .x      = (int16_t)(width > 0 ? rect.x + horizontal : rect.x + rect.width / 2),
        .y      = (int16_t)(height > 0 ? rect.y + vertical : rect.y + rect.height / 2),
        .width  = width > 0 ? width : 0,
        .height = height > 0 ? height : 0,
    };
}

void ink_ui_rect_split(ink_ui_rect_t rect, ink_ui_axis_t axis, int16_t gap, const ink_ui_length_t *lengths, size_t count,
                       ink_ui_rect_t *out_rects) {
    if (lengths == nullptr || out_rects == nullptr || count == 0) { return; }
    const int16_t  free      = free_length(axis_length(rect, axis), gap, lengths, count);
    const uint16_t weights   = flex_sum(lengths, count);
    const size_t   last_flex = last_flex_slot(lengths, count);
    int16_t        offset    = 0;
    int16_t        shared    = 0; /* free space already handed out */
    for (size_t i = 0; i < count; i++) {
        int16_t share = weights > 0 ? (int16_t)((int32_t)free * lengths[i].flex / weights) : 0;
        if (i == last_flex) { share = (int16_t)(free - shared); }
        shared               = (int16_t)(shared + share);
        const int16_t length = (int16_t)(lengths[i].fixed + share);
        out_rects[i]         = slot_rect(rect, axis, offset, length);
        offset               = (int16_t)(offset + length + gap);
    }
}

ink_ui_rect_t ink_ui_rect_align(ink_ui_rect_t outer, ink_ui_size_t size, ink_ui_align_t horizontal, ink_ui_align_t vertical) {
    return (ink_ui_rect_t){
        .x      = (int16_t)(outer.x + aligned_offset(outer.width, size.width, horizontal)),
        .y      = (int16_t)(outer.y + aligned_offset(outer.height, size.height, vertical)),
        .width  = size.width,
        .height = size.height,
    };
}
