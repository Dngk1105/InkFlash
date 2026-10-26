/* Thiết kế: docs/design/input.md */
#include "ink_gesture.h"

/* Hai lớp: debounce từng nút, rồi state machine gesture. Mọi deadline tính trong earliest(); tick xử lý deadline
 * sớm nhất trước nên gesture ra đúng thứ tự. */

#define BTN_COUNT 2 /* bên trong đánh số nút 0..BTN_COUNT-1; ra ngoài đổi sang bit */

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

enum : uint8_t {
    DL_DEBOUNCE,
    DL_HOLD,
    DL_DOUBLE,
    DL_AB_LONG,
};

typedef struct {
    bool     found;
    uint8_t  kind;
    uint8_t  btn;
    uint32_t t;
} deadline_t;

static bool reached(uint32_t now, uint32_t t)
{
    return (int32_t)(now - t) >= 0;
}

static bool config_ok(const ink_gesture_config_t *c)
{
    return c->debounce_ms < c->double_ms
        && c->debounce_ms < c->hold_ms
        && c->combo_ms < c->hold_ms
        && c->ab_short_ms > 0
        && c->ab_short_ms < c->ab_long_ms;
}

static uint8_t bit_of(uint8_t i)
{
    return (uint8_t)(1u << i);
}

/* Mã nút → số thứ tự; -1 nếu không phải đúng một nút. */
static int index_of(ink_gesture_btn_t btn)
{
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        if (btn == bit_of(i)) { return i; }
    }
    return -1;
}

static void emit(ink_gesture_out_t *out, uint8_t mod, uint8_t btn, ink_gesture_type_t type)
{
    if (out->count < INK_GESTURE_OUT_MAX) {
        out->items[out->count++] = (ink_gesture_t){
            .mod  = (ink_gesture_mod_t)mod,
            .src  = INK_GESTURE_SRC_BTN,
            .btn  = (ink_gesture_btn_t)btn,
            .type = type,
        };
    }
}

static void clear_gesture(ink_gesture_btn_state_t *b)
{
    b->phase       = b->stable ? PHASE_WAIT_RELEASE : PHASE_IDLE;
    b->mod         = INK_GESTURE_MOD_NONE;
    b->used_as_mod = false;
}

/* Lần bấm này không thành double press: phát press còn treo trước. */
static void flush_pending_press(ink_gesture_btn_state_t *b, uint8_t i, ink_gesture_out_t *out)
{
    if (b->phase == PHASE_UP_WAIT) { emit(out, b->mod, bit_of(i), INK_GESTURE_TYPE_PRESS); }
    b->phase = PHASE_IDLE;
    b->mod   = INK_GESTURE_MOD_NONE;
}

static void press_single(ink_gesture_btn_state_t *b, uint8_t i, uint32_t t, uint8_t mod, ink_gesture_out_t *out)
{
    if (b->phase == PHASE_UP_WAIT) {
        emit(out, b->mod, bit_of(i), INK_GESTURE_TYPE_DOUBLE_PRESS);
        b->phase = PHASE_WAIT_RELEASE;
        return;
    }
    b->phase  = PHASE_DOWN;
    b->t_down = t;
    b->mod    = mod;
}

static void on_press(ink_gesture_state_t *st, uint8_t i, uint32_t t, ink_gesture_out_t *out)
{
    ink_gesture_btn_state_t *b = &st->priv.btn[i];
    ink_gesture_btn_state_t *o = &st->priv.btn[1 - i];
    if (st->priv.mode != MODE_NORMAL) { return; }
    if (!o->stable) {
        press_single(b, i, t, INK_GESTURE_MOD_NONE, out);
        return;
    }
    const bool o_fresh = o->phase == PHASE_DOWN && o->mod == INK_GESTURE_MOD_NONE && !o->used_as_mod;
    if (o_fresh && t - o->t_down <= st->priv.cfg.combo_ms) {
        flush_pending_press(b, i, out);
        o->phase      = PHASE_IDLE;
        st->priv.mode = MODE_AB;
        st->priv.ab_t = t;
        return;
    }
    if (o->phase == PHASE_DOWN && (o->mask & INK_GESTURE_WAIT_MOD)) {
        o->used_as_mod = true;
        press_single(b, i, t, bit_of(1 - i), out);
        return;
    }
    flush_pending_press(b, i, out);
    clear_gesture(o);
    st->priv.mode = MODE_BLOCK;
}

static void on_release(ink_gesture_state_t *st, uint8_t i, uint32_t t, ink_gesture_out_t *out)
{
    ink_gesture_btn_state_t *b = &st->priv.btn[i];
    ink_gesture_btn_state_t *o = &st->priv.btn[1 - i];
    if (st->priv.mode != MODE_NORMAL) {
        if (o->stable) { return; }
        if (st->priv.mode == MODE_AB) {
            const bool short_hold = t - st->priv.ab_t >= st->priv.cfg.ab_short_ms;
            emit(out, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB,
                 short_hold ? INK_GESTURE_TYPE_HOLD_AB_SHORT : INK_GESTURE_TYPE_PRESS);
        }
        st->priv.mode = MODE_NORMAL;
        clear_gesture(b);
        clear_gesture(o);
        return;
    }
    if (b->used_as_mod) {
        clear_gesture(b);
        return;
    }
    if (b->phase == PHASE_DOWN) {
        if ((b->mask & INK_GESTURE_WAIT_MOD) && t - b->t_down >= st->priv.cfg.hold_ms) {
            emit(out, b->mod, bit_of(i), INK_GESTURE_TYPE_HOLD);
        } else if (b->mask & INK_GESTURE_WAIT_DOUBLE) {
            b->phase = PHASE_UP_WAIT;
            b->t_up  = t;
            return;
        } else {
            emit(out, b->mod, bit_of(i), INK_GESTURE_TYPE_PRESS);
        }
    }
    clear_gesture(b);
}

static void accept_edge(ink_gesture_state_t *st, uint8_t i, bool pressed, uint32_t t, ink_gesture_out_t *out)
{
    ink_gesture_btn_state_t *b = &st->priv.btn[i];
    b->stable                  = pressed;
    b->locked                  = true;
    b->lock_until              = t + st->priv.cfg.debounce_ms;
    if (pressed) {
        on_press(st, i, t, out);
    } else {
        on_release(st, i, t, out);
    }
}

static void consider(deadline_t *d, uint32_t t, uint8_t kind, uint8_t btn)
{
    if (!d->found || (int32_t)(t - d->t) < 0) { *d = (deadline_t){.found = true, .kind = kind, .btn = btn, .t = t}; }
}

static deadline_t earliest(const ink_gesture_state_t *st)
{
    deadline_t d = {.found = false};
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        const ink_gesture_btn_state_t *b = &st->priv.btn[i];
        if (b->locked) { consider(&d, b->lock_until, DL_DEBOUNCE, i); }
        if (st->priv.mode != MODE_NORMAL) { continue; }
        if (b->phase == PHASE_DOWN && !b->used_as_mod && !(b->mask & INK_GESTURE_WAIT_MOD)) {
            consider(&d, b->t_down + st->priv.cfg.hold_ms, DL_HOLD, i);
        }
        if (b->phase == PHASE_UP_WAIT) { consider(&d, b->t_up + st->priv.cfg.double_ms, DL_DOUBLE, i); }
    }
    if (st->priv.mode == MODE_AB) { consider(&d, st->priv.ab_t + st->priv.cfg.ab_long_ms, DL_AB_LONG, 0); }
    return d;
}

static void run_deadline(ink_gesture_state_t *st, const deadline_t *d, ink_gesture_out_t *out)
{
    ink_gesture_btn_state_t *b = &st->priv.btn[d->btn];
    switch (d->kind) {
        case DL_DEBOUNCE:
            b->locked = false;
            if (b->raw != b->stable) { accept_edge(st, d->btn, b->raw, d->t, out); }
            break;
        case DL_HOLD:
            emit(out, b->mod, bit_of(d->btn), INK_GESTURE_TYPE_HOLD);
            b->phase = PHASE_WAIT_RELEASE;
            break;
        case DL_DOUBLE:
            emit(out, b->mod, bit_of(d->btn), INK_GESTURE_TYPE_PRESS);
            clear_gesture(b);
            break;
        case DL_AB_LONG:
            emit(out, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB, INK_GESTURE_TYPE_HOLD_AB_LONG);
            st->priv.mode = MODE_AB_DONE;
            break;
    }
}

/* Mỗi deadline phát tối đa một gesture; out đầy thì dừng, phần còn lại để lần gọi sau. */
static void run_due(ink_gesture_state_t *st, uint32_t now, ink_gesture_out_t *out)
{
    while (out->count < INK_GESTURE_OUT_MAX) {
        const deadline_t d = earliest(st);
        if (!d.found || !reached(now, d.t)) { return; }
        run_deadline(st, &d, out);
    }
}

ink_gesture_err_t ink_gesture_init(ink_gesture_state_t *state, const ink_gesture_config_t *cfg)
{
    if (state == nullptr || cfg == nullptr) { return INK_GESTURE_ERR_ARG; }
    if (!config_ok(cfg)) { return INK_GESTURE_ERR_CONFIG; }
    *state          = (ink_gesture_state_t){};
    state->priv.cfg = *cfg;
    return INK_GESTURE_OK;
}

ink_gesture_err_t ink_gesture_set_config(ink_gesture_state_t *state, const ink_gesture_config_t *cfg)
{
    if (state == nullptr || cfg == nullptr) { return INK_GESTURE_ERR_ARG; }
    if (!config_ok(cfg)) { return INK_GESTURE_ERR_CONFIG; }
    state->priv.cfg = *cfg;
    return INK_GESTURE_OK;
}

ink_gesture_err_t ink_gesture_get_config(const ink_gesture_state_t *state, ink_gesture_config_t *out)
{
    if (state == nullptr || out == nullptr) { return INK_GESTURE_ERR_ARG; }
    *out = state->priv.cfg;
    return INK_GESTURE_OK;
}

ink_gesture_err_t ink_gesture_set_enabled(ink_gesture_state_t *state, ink_gesture_btn_t btn, uint8_t mask)
{
    const int i = index_of(btn);
    if (state == nullptr || i < 0 || (mask & ~(INK_GESTURE_WAIT_DOUBLE | INK_GESTURE_WAIT_MOD)) != 0) {
        return INK_GESTURE_ERR_ARG;
    }
    ink_gesture_btn_state_t *b = &state->priv.btn[i];
    if (b->mask == mask) { return INK_GESTURE_OK; }
    b->mask = mask;
    if (state->priv.mode == MODE_NORMAL && (b->phase != PHASE_IDLE || b->used_as_mod)) { clear_gesture(b); }
    return INK_GESTURE_OK;
}

ink_gesture_err_t ink_gesture_feed(ink_gesture_state_t *state, const ink_gesture_event_t *event, ink_gesture_out_t *out)
{
    if (state == nullptr || event == nullptr || out == nullptr || event->level > INK_GESTURE_LEVEL_PRESSED) {
        return INK_GESTURE_ERR_ARG;
    }
    const int i = index_of(event->btn);
    if (i < 0) { return INK_GESTURE_ERR_ARG; }
    out->count = 0;
    run_due(state, event->time_ms, out);
    ink_gesture_btn_state_t *b       = &state->priv.btn[i];
    const bool               pressed = event->level == INK_GESTURE_LEVEL_PRESSED;
    b->raw                           = pressed;
    if (!b->locked && pressed != b->stable) { accept_edge(state, (uint8_t)i, pressed, event->time_ms, out); }
    return INK_GESTURE_OK;
}

ink_gesture_err_t ink_gesture_tick(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_out_t *out)
{
    if (state == nullptr || out == nullptr) { return INK_GESTURE_ERR_ARG; }
    out->count = 0;
    run_due(state, now_ms, out);
    return INK_GESTURE_OK;
}

bool ink_gesture_next_deadline(const ink_gesture_state_t *state, uint32_t *out_ms)
{
    if (state == nullptr || out_ms == nullptr) { return false; }
    const deadline_t d = earliest(state);
    if (d.found) { *out_ms = d.t; }
    return d.found;
}

bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out)
{
    if (state == nullptr || out == nullptr) { return false; }
    const ink_gesture_config_t *cfg = &state->priv.cfg;
    if (state->priv.mode == MODE_AB) {
        const uint32_t ab_t       = state->priv.ab_t;
        const bool     past_short = reached(now_ms, ab_t + cfg->ab_short_ms);

        *out = (ink_gesture_progress_t){
            .mod        = INK_GESTURE_MOD_NONE,
            .btn        = INK_GESTURE_BTN_AB,
            .on_release = past_short ? INK_GESTURE_TYPE_HOLD_AB_SHORT : INK_GESTURE_TYPE_PRESS,
            .has_next   = true,
            .next       = past_short ? INK_GESTURE_TYPE_HOLD_AB_LONG : INK_GESTURE_TYPE_HOLD_AB_SHORT,
            .start_ms   = ab_t,
            .next_ms    = ab_t + (past_short ? cfg->ab_long_ms : cfg->ab_short_ms),
        };
        return true;
    }
    if (state->priv.mode != MODE_NORMAL) { return false; }
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        const ink_gesture_btn_state_t *b = &state->priv.btn[i];
        if (b->phase != PHASE_DOWN || b->used_as_mod) { continue; }
        const uint32_t hold_at   = b->t_down + cfg->hold_ms;
        const bool     past_hold = reached(now_ms, hold_at);

        *out = (ink_gesture_progress_t){
            .mod        = (ink_gesture_mod_t)b->mod,
            .btn        = (ink_gesture_btn_t)bit_of(i),
            .on_release = past_hold ? INK_GESTURE_TYPE_HOLD : INK_GESTURE_TYPE_PRESS,
            .has_next   = !past_hold,
            .next       = INK_GESTURE_TYPE_HOLD,
            .start_ms   = b->t_down,
            .next_ms    = hold_at,
        };
        return true;
    }
    return false;
}
