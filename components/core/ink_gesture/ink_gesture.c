/* Thiết kế: docs/design/input.md */
#include "ink_gesture.h"

/* Hai lớp: debounce từng nút, rồi state machine gesture. Mọi deadline tính
 * trong earliest_deadline(); tick xử lý deadline sớm nhất trước nên gesture ra
 * đúng thứ tự. */

/* ---- Trạng thái nội bộ ---- */

/* bên trong đánh số nút 0..BUTTON_COUNT-1; ra ngoài đổi sang bit */
#define BUTTON_COUNT 2

enum : uint8_t {
    PHASE_IDLE,         /* nhả, không có gì dở */
    PHASE_DOWN,         /* đang bấm, chưa biết press hay hold */
    PHASE_UP_WAIT,      /* đã nhả, chờ lần bấm thứ hai */
    PHASE_WAIT_RELEASE, /* chờ nhả, không phát gì */
};

enum : uint8_t {
    MODE_NORMAL,
    MODE_AB,      /* đang giữ A+B */
    MODE_AB_DONE, /* đã phát HOLD_AB_LONG, chờ nhả hết */
    MODE_BLOCK,   /* bỏ qua tới khi nhả hết */
};

/* ---- Tiện ích ---- */

static bool time_reached(uint32_t now_ms, uint32_t time_ms) {
    return (int32_t)(now_ms - time_ms) >= 0;
}

static bool config_valid(const ink_gesture_config_t *config) {
    return config->debounce_ms < config->double_ms
        && config->debounce_ms < config->hold_ms
        && config->combo_ms < config->hold_ms
        && config->ab_short_ms > 0
        && config->ab_short_ms < config->ab_long_ms;
}

static uint8_t button_bit(uint8_t button_index) {
    return (uint8_t)(1u << button_index);
}

/* Mã nút → số thứ tự (A = 1 → 0); -1 nếu không phải đúng một nút. */
static int button_index_of(ink_gesture_button_t button) {
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        if (button == button_bit(i)) { return i; }
    }
    return -1;
}

static void emit(ink_gesture_output_t *out, uint8_t modifier, uint8_t button, ink_gesture_type_t type) {
    if (out->count < INK_GESTURE_OUTPUT_MAX) {
        out->items[out->count++] = (ink_gesture_t){
            .modifier = (ink_gesture_modifier_t)modifier,
            .source   = INK_GESTURE_SOURCE_BUTTON,
            .button   = (ink_gesture_button_t)button,
            .type     = type,
        };
    }
}

/* ---- State machine: bấm, nhả ---- */

static void clear_gesture(ink_gesture_button_state_t *button) {
    button->phase            = button->stable ? PHASE_WAIT_RELEASE : PHASE_IDLE;
    button->modifier         = INK_GESTURE_MODIFIER_NONE;
    button->used_as_modifier = false;
}

/* Lần bấm này không thành double press: phát press còn treo trước. */
static void flush_pending_press(ink_gesture_button_state_t *button, uint8_t button_index, ink_gesture_output_t *out) {
    if (button->phase == PHASE_UP_WAIT) { emit(out, button->modifier, button_bit(button_index), INK_GESTURE_TYPE_PRESS); }
    button->phase    = PHASE_IDLE;
    button->modifier = INK_GESTURE_MODIFIER_NONE;
}

static void press_single(ink_gesture_button_state_t *button, uint8_t button_index, uint32_t time_ms, uint8_t modifier,
                         ink_gesture_output_t *out) {
    if (button->phase == PHASE_UP_WAIT) {
        emit(out, button->modifier, button_bit(button_index), INK_GESTURE_TYPE_DOUBLE_PRESS);
        button->phase = PHASE_WAIT_RELEASE;
        return;
    }
    button->phase         = PHASE_DOWN;
    button->pressed_at_ms = time_ms;
    button->modifier      = modifier;
}

static void on_press(ink_gesture_state_t *state, uint8_t button_index, uint32_t time_ms, ink_gesture_output_t *out) {
    ink_gesture_button_state_t *button = &state->priv.buttons[button_index];
    ink_gesture_button_state_t *other  = &state->priv.buttons[1 - button_index];
    if (state->priv.mode != MODE_NORMAL) { return; }
    if (!other->stable) {
        press_single(button, button_index, time_ms, INK_GESTURE_MODIFIER_NONE, out);
        return;
    }
    const bool other_fresh = other->phase == PHASE_DOWN && other->modifier == INK_GESTURE_MODIFIER_NONE && !other->used_as_modifier;
    if (other_fresh && time_ms - other->pressed_at_ms <= state->priv.config.combo_ms) {
        flush_pending_press(button, button_index, out);
        other->phase                 = PHASE_IDLE;
        state->priv.mode             = MODE_AB;
        state->priv.ab_pressed_at_ms = time_ms;
        return;
    }
    if (other->phase == PHASE_DOWN && (other->wait_mask & INK_GESTURE_WAIT_MODIFIER)) {
        other->used_as_modifier = true;
        press_single(button, button_index, time_ms, button_bit(1 - button_index), out);
        return;
    }
    flush_pending_press(button, button_index, out);
    clear_gesture(other);
    state->priv.mode = MODE_BLOCK;
}

static void on_release(ink_gesture_state_t *state, uint8_t button_index, uint32_t time_ms, ink_gesture_output_t *out) {
    const ink_gesture_config_t *config = &state->priv.config;
    ink_gesture_button_state_t *button = &state->priv.buttons[button_index];
    ink_gesture_button_state_t *other  = &state->priv.buttons[1 - button_index];
    if (state->priv.mode != MODE_NORMAL) {
        if (other->stable) { return; }
        if (state->priv.mode == MODE_AB) {
            const bool held_short = time_ms - state->priv.ab_pressed_at_ms >= config->ab_short_ms;
            emit(out, INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_AB,
                 held_short ? INK_GESTURE_TYPE_HOLD_AB_SHORT : INK_GESTURE_TYPE_PRESS);
        }
        state->priv.mode = MODE_NORMAL;
        clear_gesture(button);
        clear_gesture(other);
        return;
    }
    if (button->used_as_modifier) {
        clear_gesture(button);
        return;
    }
    if (button->phase == PHASE_DOWN) {
        const bool held = time_ms - button->pressed_at_ms >= config->hold_ms;
        if ((button->wait_mask & INK_GESTURE_WAIT_MODIFIER) && held) {
            emit(out, button->modifier, button_bit(button_index), INK_GESTURE_TYPE_HOLD);
        } else if (button->wait_mask & INK_GESTURE_WAIT_DOUBLE) {
            button->phase          = PHASE_UP_WAIT;
            button->released_at_ms = time_ms;
            return;
        } else {
            emit(out, button->modifier, button_bit(button_index), INK_GESTURE_TYPE_PRESS);
        }
    }
    clear_gesture(button);
}

/* ---- Debounce ---- */

static void accept_edge(ink_gesture_state_t *state, uint8_t button_index, bool pressed, uint32_t time_ms, ink_gesture_output_t *out) {
    ink_gesture_button_state_t *button = &state->priv.buttons[button_index];
    button->stable                     = pressed;
    button->locked                     = true;
    button->lock_until_ms              = time_ms + state->priv.config.debounce_ms;
    if (pressed) {
        on_press(state, button_index, time_ms, out);
    } else {
        on_release(state, button_index, time_ms, out);
    }
}

/* ---- Deadline ---- */

enum : uint8_t {
    DEADLINE_DEBOUNCE,
    DEADLINE_HOLD,
    DEADLINE_DOUBLE,
    DEADLINE_AB_LONG,
};

typedef struct {
    bool     found;
    uint8_t  kind;
    uint8_t  button_index;
    uint32_t time_ms;
} deadline_t;

static void keep_earlier(deadline_t *deadline, uint32_t time_ms, uint8_t kind, uint8_t button_index) {
    if (!deadline->found || (int32_t)(time_ms - deadline->time_ms) < 0) {
        *deadline = (deadline_t){.found = true, .kind = kind, .button_index = button_index, .time_ms = time_ms};
    }
}

static deadline_t earliest_deadline(const ink_gesture_state_t *state) {
    const ink_gesture_config_t *config   = &state->priv.config;
    deadline_t                  deadline = {.found = false};
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        const ink_gesture_button_state_t *button = &state->priv.buttons[i];
        if (button->locked) { keep_earlier(&deadline, button->lock_until_ms, DEADLINE_DEBOUNCE, i); }
        if (state->priv.mode != MODE_NORMAL) { continue; }
        if (button->phase == PHASE_DOWN && !button->used_as_modifier && !(button->wait_mask & INK_GESTURE_WAIT_MODIFIER)) {
            keep_earlier(&deadline, button->pressed_at_ms + config->hold_ms, DEADLINE_HOLD, i);
        }
        if (button->phase == PHASE_UP_WAIT) { keep_earlier(&deadline, button->released_at_ms + config->double_ms, DEADLINE_DOUBLE, i); }
    }
    if (state->priv.mode == MODE_AB) { keep_earlier(&deadline, state->priv.ab_pressed_at_ms + config->ab_long_ms, DEADLINE_AB_LONG, 0); }
    return deadline;
}

static void run_deadline(ink_gesture_state_t *state, const deadline_t *deadline, ink_gesture_output_t *out) {
    ink_gesture_button_state_t *button = &state->priv.buttons[deadline->button_index];
    switch (deadline->kind) {
        case DEADLINE_DEBOUNCE:
            button->locked = false;
            if (button->raw != button->stable) { accept_edge(state, deadline->button_index, button->raw, deadline->time_ms, out); }
            break;
        case DEADLINE_HOLD:
            emit(out, button->modifier, button_bit(deadline->button_index), INK_GESTURE_TYPE_HOLD);
            button->phase = PHASE_WAIT_RELEASE;
            break;
        case DEADLINE_DOUBLE:
            emit(out, button->modifier, button_bit(deadline->button_index), INK_GESTURE_TYPE_PRESS);
            clear_gesture(button);
            break;
        case DEADLINE_AB_LONG:
            emit(out, INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_AB, INK_GESTURE_TYPE_HOLD_AB_LONG);
            state->priv.mode = MODE_AB_DONE;
            break;
    }
}

/* Mỗi deadline phát tối đa một gesture; out đầy thì dừng, phần còn lại để lần
 * gọi sau. */
static void run_due_deadlines(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_output_t *out) {
    while (out->count < INK_GESTURE_OUTPUT_MAX) {
        const deadline_t deadline = earliest_deadline(state);
        if (!deadline.found || !time_reached(now_ms, deadline.time_ms)) { return; }
        run_deadline(state, &deadline, out);
    }
}

/* ---- API: cấu hình ---- */

ink_gesture_error_t ink_gesture_init(ink_gesture_state_t *state, const ink_gesture_config_t *config) {
    if (state == nullptr || config == nullptr) { return INK_GESTURE_ERROR_ARGUMENT; }
    if (!config_valid(config)) { return INK_GESTURE_ERROR_CONFIG; }
    *state             = (ink_gesture_state_t){};
    state->priv.config = *config;
    return INK_GESTURE_OK;
}

ink_gesture_error_t ink_gesture_set_config(ink_gesture_state_t *state, const ink_gesture_config_t *config) {
    if (state == nullptr || config == nullptr) { return INK_GESTURE_ERROR_ARGUMENT; }
    if (!config_valid(config)) { return INK_GESTURE_ERROR_CONFIG; }
    state->priv.config = *config;
    return INK_GESTURE_OK;
}

ink_gesture_error_t ink_gesture_get_config(const ink_gesture_state_t *state, ink_gesture_config_t *out_config) {
    if (state == nullptr || out_config == nullptr) { return INK_GESTURE_ERROR_ARGUMENT; }
    *out_config = state->priv.config;
    return INK_GESTURE_OK;
}

ink_gesture_error_t ink_gesture_set_enabled(ink_gesture_state_t *state, ink_gesture_button_t button, uint8_t wait_mask) {
    const int button_index = button_index_of(button);
    if (state == nullptr || button_index < 0 || (wait_mask & ~(INK_GESTURE_WAIT_DOUBLE | INK_GESTURE_WAIT_MODIFIER)) != 0) {
        return INK_GESTURE_ERROR_ARGUMENT;
    }
    ink_gesture_button_state_t *button_state = &state->priv.buttons[button_index];
    if (button_state->wait_mask == wait_mask) { return INK_GESTURE_OK; }
    button_state->wait_mask = wait_mask;
    if (state->priv.mode == MODE_NORMAL && (button_state->phase != PHASE_IDLE || button_state->used_as_modifier)) {
        clear_gesture(button_state);
    }
    return INK_GESTURE_OK;
}

/* ---- API: edge và tick ---- */

ink_gesture_error_t ink_gesture_feed(ink_gesture_state_t *state, const ink_gesture_event_t *event, ink_gesture_output_t *out) {
    if (state == nullptr || event == nullptr || out == nullptr || event->level > INK_GESTURE_LEVEL_PRESSED) {
        return INK_GESTURE_ERROR_ARGUMENT;
    }
    const int button_index = button_index_of(event->button);
    if (button_index < 0) { return INK_GESTURE_ERROR_ARGUMENT; }
    out->count = 0;
    run_due_deadlines(state, event->time_ms, out);
    ink_gesture_button_state_t *button  = &state->priv.buttons[button_index];
    const bool                  pressed = event->level == INK_GESTURE_LEVEL_PRESSED;
    button->raw                         = pressed;
    if (!button->locked && pressed != button->stable) { accept_edge(state, (uint8_t)button_index, pressed, event->time_ms, out); }
    return INK_GESTURE_OK;
}

ink_gesture_error_t ink_gesture_tick(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_output_t *out) {
    if (state == nullptr || out == nullptr) { return INK_GESTURE_ERROR_ARGUMENT; }
    out->count = 0;
    run_due_deadlines(state, now_ms, out);
    return INK_GESTURE_OK;
}

bool ink_gesture_next_deadline(const ink_gesture_state_t *state, uint32_t *out_ms) {
    if (state == nullptr || out_ms == nullptr) { return false; }
    const deadline_t deadline = earliest_deadline(state);
    if (deadline.found) { *out_ms = deadline.time_ms; }
    return deadline.found;
}

/* ---- API: progress ---- */

bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out) {
    if (state == nullptr || out == nullptr) { return false; }
    const ink_gesture_config_t *config = &state->priv.config;
    if (state->priv.mode == MODE_AB) {
        const uint32_t ab_pressed_at_ms = state->priv.ab_pressed_at_ms;
        const bool     past_short       = time_reached(now_ms, ab_pressed_at_ms + config->ab_short_ms);

        *out = (ink_gesture_progress_t){
            .modifier   = INK_GESTURE_MODIFIER_NONE,
            .button     = INK_GESTURE_BUTTON_AB,
            .on_release = past_short ? INK_GESTURE_TYPE_HOLD_AB_SHORT : INK_GESTURE_TYPE_PRESS,
            .has_next   = true,
            .next       = past_short ? INK_GESTURE_TYPE_HOLD_AB_LONG : INK_GESTURE_TYPE_HOLD_AB_SHORT,
            .start_ms   = ab_pressed_at_ms,
            .next_ms    = ab_pressed_at_ms + (past_short ? config->ab_long_ms : config->ab_short_ms),
        };
        return true;
    }
    if (state->priv.mode != MODE_NORMAL) { return false; }
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        const ink_gesture_button_state_t *button = &state->priv.buttons[i];
        if (button->phase != PHASE_DOWN || button->used_as_modifier) { continue; }
        const uint32_t hold_at_ms = button->pressed_at_ms + config->hold_ms;
        const bool     past_hold  = time_reached(now_ms, hold_at_ms);

        *out = (ink_gesture_progress_t){
            .modifier   = (ink_gesture_modifier_t)button->modifier,
            .button     = (ink_gesture_button_t)button_bit(i),
            .on_release = past_hold ? INK_GESTURE_TYPE_HOLD : INK_GESTURE_TYPE_PRESS,
            .has_next   = !past_hold,
            .next       = INK_GESTURE_TYPE_HOLD,
            .start_ms   = button->pressed_at_ms,
            .next_ms    = hold_at_ms,
        };
        return true;
    }
    return false;
}
