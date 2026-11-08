/* Design: docs/design/display/display.md */
#include "ink_display_policy.h"

#include <string.h>

/* pixel_state byte of one pixel. */
#define FLAG_MASK  0x03U
#define FLAG_NONE  0x00U
#define FLAG_DARK  0x01U /* dark edge: unchanged white pixel next to a pixel that just left black */
#define FLAG_LIGHT 0x02U /* light edge: unchanged black pixel next to a pixel that just turned black */
#define DEBT_SHIFT 2
#define DEBT_MASK  (0x0FU << DEBT_SHIFT)

/* Bits of stages_done. */
#define STAGE_EDGE   (1U << 0)
#define STAGE_LIGHT  (1U << 1)
#define STAGE_STRONG (1U << 2)

static bool config_valid(const ink_display_policy_config_t *config) {
    return config->width != 0
        && config->height != 0
        && config->x_align != 0
        && config->auto_ui_percent <= 100
        && config->debt_light != 0
        && config->debt_ceiling != 0
        && config->debt_ceiling <= config->debt_ceiling_max
        && config->debt_ceiling_max <= INK_DISPLAY_POLICY_MAX_DEBT
        && config->debt_weights[INK_DISPLAY_INTENT_TEXT] <= INK_DISPLAY_POLICY_MAX_DEBT
        && config->debt_weights[INK_DISPLAY_INTENT_UI] <= INK_DISPLAY_POLICY_MAX_DEBT
        && config->debt_weights[INK_DISPLAY_INTENT_PAGE] <= INK_DISPLAY_POLICY_MAX_DEBT;
}

static size_t pixel_count(const ink_display_policy_config_t *config) {
    return (size_t)config->width * config->height;
}

/* Every pixel: debt 0, no edge flag; restart the cleanup stages. */
static void clear_pixels(ink_display_policy_state_t *state) {
    memset(state->priv.pixel_state, 0, pixel_count(&state->priv.config));
    state->priv.flagged_count = 0;
    state->priv.max_debt      = 0;
}

ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t *state, const ink_display_policy_config_t *config,
                                                   uint8_t *pixel_state) {
    if (state == nullptr || config == nullptr || pixel_state == nullptr || !config_valid(config)) {
        return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
    }
    *state                  = (ink_display_policy_state_t){};
    state->priv.config      = *config;
    state->priv.pixel_state = pixel_state;
    state->priv.ceiling     = config->debt_ceiling;
    clear_pixels(state);
    return INK_DISPLAY_POLICY_OK;
}

ink_display_policy_error_t ink_display_policy_set_config(ink_display_policy_state_t *state, const ink_display_policy_config_t *config) {
    if (state == nullptr || config == nullptr || !config_valid(config)) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    const ink_display_policy_config_t *old = &state->priv.config;
    if (config->width != old->width || config->height != old->height || config->x_align != old->x_align) {
        return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
    }
    state->priv.config = *config;
    if (state->priv.ceiling < config->debt_ceiling) { state->priv.ceiling = config->debt_ceiling; }
    if (state->priv.ceiling > config->debt_ceiling_max) { state->priv.ceiling = config->debt_ceiling_max; }
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

uint8_t ink_display_policy_max_debt(const ink_display_policy_state_t *state) {
    return state == nullptr ? 0 : state->priv.max_debt;
}

uint32_t ink_display_policy_flagged_count(const ink_display_policy_state_t *state) {
    return state == nullptr ? 0 : state->priv.flagged_count;
}

uint8_t ink_display_policy_ceiling(const ink_display_policy_state_t *state) {
    return state == nullptr ? 0 : state->priv.ceiling;
}

/* ---- 1-bit frames ---- */

static size_t row_bytes(const ink_display_policy_config_t *config) {
    return ((size_t)config->width + 7) / 8;
}

/* Bit of pixel `pixel` (0..7) in its byte; leftmost pixel in the most significant bit. */
static uint8_t pixel_bit(uint32_t pixel) {
    return (uint8_t)(0x80U >> pixel);
}

static bool is_white(const ink_display_policy_config_t *config, const uint8_t *frame, uint32_t x, uint32_t y) {
    return (frame[y * row_bytes(config) + x / 8] & pixel_bit(x % 8)) != 0;
}

static void invert_pixel(const ink_display_policy_config_t *config, uint8_t *frame, uint32_t x, uint32_t y) {
    frame[y * row_bytes(config) + x / 8] ^= pixel_bit(x % 8);
}

/* Differing bits of a byte between two frames; padding bits at row end masked out. */
static uint8_t diff_byte(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next, uint32_t y, size_t i) {
    const size_t offset = y * row_bytes(config) + i;
    uint8_t      diff   = shown[offset] ^ next[offset];
    if (i == row_bytes(config) - 1 && config->width % 8 != 0) { diff &= (uint8_t)(0xFF << (8 - config->width % 8)); }
    return diff;
}

typedef struct {
    uint32_t left; /* all edges inclusive */
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
} bounds_t;

static bounds_t empty_bounds(void) {
    return (bounds_t){.left = UINT32_MAX, .right = 0, .top = UINT32_MAX, .bottom = 0};
}

static void add_to_bounds(bounds_t *bounds, uint32_t x, uint32_t y) {
    if (x < bounds->left) { bounds->left = x; }
    if (x > bounds->right) { bounds->right = x; }
    if (y < bounds->top) { bounds->top = y; }
    if (y > bounds->bottom) { bounds->bottom = y; }
}

static bool bounds_empty(const bounds_t *bounds) {
    return bounds->top == UINT32_MAX;
}

/* Bounds of the differing pixels; false if the frames are identical. */
static bool find_changed_bounds(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next,
                                bounds_t *out_bounds) {
    bounds_t bounds = empty_bounds();
    for (uint32_t y = 0; y < config->height; y++) {
        for (size_t i = 0; i < row_bytes(config); i++) {
            const uint8_t diff = diff_byte(config, shown, next, y, i);
            if (diff == 0) { continue; }
            for (uint32_t pixel = 0; pixel < 8; pixel++) {
                if ((diff & pixel_bit(pixel)) != 0) { add_to_bounds(&bounds, (uint32_t)i * 8 + pixel, y); }
            }
        }
    }
    if (bounds_empty(&bounds)) { return false; }
    *out_bounds = bounds;
    return true;
}

/* Bounds -> window, x edges aligned to x_align. */
static ink_display_rect_t make_window(const ink_display_policy_config_t *config, bounds_t bounds) {
    const uint32_t align        = config->x_align;
    const uint32_t aligned_left = bounds.left / align * align;
    uint32_t       right        = (bounds.right + 1 + align - 1) / align * align; /* right edge, exclusive */
    if (right > config->width) { right = config->width; }
    return (ink_display_rect_t){
        .x      = (uint16_t)aligned_left,
        .y      = (uint16_t)bounds.top,
        .width  = (uint16_t)(right - aligned_left),
        .height = (uint16_t)(bounds.bottom + 1 - bounds.top),
    };
}

static ink_display_rect_t whole_screen(const ink_display_policy_config_t *config) {
    return (ink_display_rect_t){.x = 0, .y = 0, .width = config->width, .height = config->height};
}

bool ink_display_policy_changed_window(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next,
                                       ink_display_rect_t *out_window) {
    if (config == nullptr || shown == nullptr || next == nullptr || out_window == nullptr || config->x_align == 0) { return false; }
    bounds_t bounds;
    if (!find_changed_bounds(config, shown, next, &bounds)) { return false; }
    *out_window = make_window(config, bounds);
    return true;
}

/* ---- Per-pixel state ---- */

static uint8_t *pixel_state(const ink_display_policy_state_t *state, uint32_t x, uint32_t y) {
    return &state->priv.pixel_state[(size_t)y * state->priv.config.width + x];
}

static uint8_t debt_of(uint8_t pixel) {
    return (uint8_t)((pixel & DEBT_MASK) >> DEBT_SHIFT);
}

static uint8_t flag_of(uint8_t pixel) {
    return pixel & FLAG_MASK;
}

static void set_debt(uint8_t *pixel, uint32_t debt) {
    if (debt > INK_DISPLAY_POLICY_MAX_DEBT) { debt = INK_DISPLAY_POLICY_MAX_DEBT; }
    *pixel = (uint8_t)((*pixel & ~DEBT_MASK) | (debt << DEBT_SHIFT));
}

static void set_flag(ink_display_policy_state_t *state, uint8_t *pixel, uint8_t flag) {
    if (flag_of(*pixel) != FLAG_NONE) { state->priv.flagged_count--; }
    if (flag != FLAG_NONE) { state->priv.flagged_count++; }
    *pixel = (uint8_t)((*pixel & ~FLAG_MASK) | flag);
}

static void recompute_max_debt(ink_display_policy_state_t *state) {
    uint8_t      max_debt = 0;
    const size_t count    = pixel_count(&state->priv.config);
    for (size_t i = 0; i < count; i++) {
        const uint8_t debt = debt_of(state->priv.pixel_state[i]);
        if (debt > max_debt) { max_debt = debt; }
    }
    state->priv.max_debt = max_debt;
}

/* Changed pixels are walked per byte: an unchanged byte skips all 8 pixels (PSRAM is slow; their state is not read). */

/* Some changed pixel in window would exceed `ceiling` after adding `weight`. */
static bool would_exceed(const ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next, bounds_t bounds,
                         uint32_t weight, uint32_t ceiling) {
    const ink_display_policy_config_t *config = &state->priv.config;
    for (uint32_t y = bounds.top; y <= bounds.bottom; y++) {
        for (size_t i = bounds.left / 8; i <= bounds.right / 8; i++) {
            const uint8_t diff = diff_byte(config, shown, next, y, i);
            if (diff == 0) { continue; }
            for (uint32_t pixel = 0; pixel < 8; pixel++) {
                const bool changed = (diff & pixel_bit(pixel)) != 0;
                if (changed && debt_of(*pixel_state(state, (uint32_t)i * 8 + pixel, y)) + weight > ceiling) { return true; }
            }
        }
    }
    return false;
}

/* Changed pixel: add debt, clear its old edge flag (the waveform just drove it again). */
static void add_debt(ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next, bounds_t bounds, uint32_t weight) {
    const ink_display_policy_config_t *config = &state->priv.config;
    for (uint32_t y = bounds.top; y <= bounds.bottom; y++) {
        for (size_t i = bounds.left / 8; i <= bounds.right / 8; i++) {
            const uint8_t diff = diff_byte(config, shown, next, y, i);
            for (uint32_t pixel = 0; pixel < 8 && diff != 0; pixel++) {
                if ((diff & pixel_bit(pixel)) == 0) { continue; }
                uint8_t *state_pixel = pixel_state(state, (uint32_t)i * 8 + pixel, y);
                set_flag(state, state_pixel, FLAG_NONE);
                set_debt(state_pixel, debt_of(*state_pixel) + weight);
                if (debt_of(*state_pixel) > state->priv.max_debt) { state->priv.max_debt = debt_of(*state_pixel); }
            }
        }
    }
}

/* ---- Edge flags (US 11,568,827, col. 11) ---- */

static const int32_t s_cardinal[4][2] = {
    {0,  -1},
    {-1, 0 },
    {1,  0 },
    {0,  1 }
};

/* Pixel (x + dx, y + dy) is on the panel. */
static bool neighbor(const ink_display_policy_config_t *config, uint32_t x, uint32_t y, int32_t dx, int32_t dy, uint32_t *out_x,
                     uint32_t *out_y) {
    const int64_t nx = (int64_t)x + dx;
    const int64_t ny = (int64_t)y + dy;
    if (nx < 0 || ny < 0 || nx >= config->width || ny >= config->height) { return false; }
    *out_x = (uint32_t)nx;
    *out_y = (uint32_t)ny;
    return true;
}

/* The 8 neighbours, clipped to the panel; reads 3 rows of priv.pixel_state directly. */
static bool any_flag_around(const ink_display_policy_state_t *state, uint32_t x, uint32_t y) {
    const ink_display_policy_config_t *config = &state->priv.config;
    const uint32_t                     left   = x > 0 ? x - 1 : 0;
    const uint32_t                     right  = x + 1 < config->width ? x + 1 : x;
    const uint32_t                     top    = y > 0 ? y - 1 : 0;
    const uint32_t                     bottom = y + 1 < config->height ? y + 1 : y;
    for (uint32_t row = top; row <= bottom; row++) {
        const uint8_t *pixel_state = &state->priv.pixel_state[(size_t)row * config->width];
        for (uint32_t column = left; column <= right; column++) {
            if ((row != y || column != x) && flag_of(pixel_state[column]) != FLAG_NONE) { return true; }
        }
    }
    return false;
}

/* Edge flag for the unchanged pixel (x, y), or FLAG_NONE. */
static uint8_t edge_flag(const ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next, uint32_t x, uint32_t y) {
    const ink_display_policy_config_t *config         = &state->priv.config;
    const bool                         white          = is_white(config, next, x, y);
    bool                               all_next_white = true, any_shown_not_white = false, any_turned_black = false;
    for (size_t i = 0; i < 4; i++) {
        uint32_t nx, ny;
        if (!neighbor(config, x, y, s_cardinal[i][0], s_cardinal[i][1], &nx, &ny)) { continue; }
        const bool next_white  = is_white(config, next, nx, ny);
        const bool shown_white = is_white(config, shown, nx, ny);
        all_next_white         = all_next_white && next_white;
        any_shown_not_white    = any_shown_not_white || !shown_white;
        any_turned_black       = any_turned_black || (shown_white && !next_white);
    }
    if (white && all_next_white && any_shown_not_white) { return FLAG_DARK; }
    if (!white && any_turned_black) { return FLAG_LIGHT; }
    return FLAG_NONE;
}

/* Differing bits of row y, byte i; 0 outside the panel. */
static uint8_t diff_at(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next, int64_t y, int64_t i) {
    if (y < 0 || y >= config->height || i < 0 || i >= (int64_t)row_bytes(config)) { return 0; }
    return diff_byte(config, shown, next, (uint32_t)y, (size_t)i);
}

/* Pixels of the byte with at least one changed 4-neighbour: row above, row below, left, right. */
static uint8_t near_change(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next, uint32_t y, size_t i) {
    const uint8_t here  = diff_at(config, shown, next, y, (int64_t)i);
    const uint8_t left  = diff_at(config, shown, next, y, (int64_t)i - 1);
    const uint8_t right = diff_at(config, shown, next, y, (int64_t)i + 1);
    /* pixel x + 1 shifted to x, and pixel x - 1 shifted to x */
    const uint8_t sideways = (uint8_t)((here << 1) | (right >> 7)) | (uint8_t)((here >> 1) | (left << 7));
    return (uint8_t)(diff_at(config, shown, next, (int64_t)y - 1, (int64_t)i)
                     | diff_at(config, shown, next, (int64_t)y + 1, (int64_t)i)
                     | sideways);
}

/*
 * Mark row by row inside the changed window grown by 1 px; only unchanged pixels with no flag and no flagged
 * neighbour. Both flag rules need a changed 4-neighbour, so only unchanged pixels next to a changed one are checked.
 */
static void mark_edges(ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next, bounds_t bounds) {
    const ink_display_policy_config_t *config = &state->priv.config;
    const uint32_t                     top    = bounds.top > 0 ? bounds.top - 1 : 0;
    const uint32_t                     left   = bounds.left > 0 ? bounds.left - 1 : 0;
    const uint32_t                     bottom = bounds.bottom + 1 < config->height ? bounds.bottom + 1 : config->height - 1U;
    const uint32_t                     right  = bounds.right + 1 < config->width ? bounds.right + 1 : config->width - 1U;
    for (uint32_t y = top; y <= bottom; y++) {
        for (size_t i = left / 8; i <= right / 8; i++) {
            const uint8_t candidates = (uint8_t)(near_change(config, shown, next, y, i) & ~diff_at(config, shown, next, y, (int64_t)i));
            for (uint32_t pixel = 0; pixel < 8 && candidates != 0; pixel++) {
                const uint32_t x = (uint32_t)i * 8 + pixel;
                if ((candidates & pixel_bit(pixel)) == 0 || x < left || x > right) { continue; }
                uint8_t *state_pixel = pixel_state(state, x, y);
                if (flag_of(*state_pixel) != FLAG_NONE || any_flag_around(state, x, y)) { continue; }
                const uint8_t flag = edge_flag(state, shown, next, x, y);
                if (flag != FLAG_NONE) { set_flag(state, state_pixel, flag); }
            }
        }
    }
}

/* ---- Decide ---- */

static uint32_t elapsed_ms(uint32_t from_ms, uint32_t to_ms) {
    return (uint32_t)(to_ms - from_ms);
}

static bool request_valid(ink_display_policy_request_t request) {
    return request.intent < INK_DISPLAY_INTENT_COUNT;
}

/* New frame: restart the cleanup stages; a frame in the active period raises the ceiling. Returns true when active. */
static bool start_idle_period(ink_display_policy_state_t *state, ink_display_policy_request_t request, uint32_t now_ms) {
    const ink_display_policy_config_t *config = &state->priv.config;
    const bool active = state->priv.has_frame && config->active_ms > 0 && elapsed_ms(state->priv.last_frame_ms, now_ms) < config->active_ms;
    if (!active) {
        state->priv.ceiling = config->debt_ceiling;
    } else if (state->priv.ceiling < config->debt_ceiling_max) {
        state->priv.ceiling++;
    }
    state->priv.last_frame_ms    = now_ms;
    state->priv.has_frame        = true;
    state->priv.stages_done      = 0;
    state->priv.edge_center_done = false;
    state->priv.defer            = (request.flags & INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP) != 0;
    return active;
}

static ink_display_intent_t resolve_intent(const ink_display_policy_config_t *config, ink_display_intent_t intent,
                                           ink_display_rect_t window) {
    if (intent != INK_DISPLAY_INTENT_AUTO) { return intent; }
    const uint64_t area   = (uint64_t)window.width * window.height;
    const uint64_t screen = (uint64_t)config->width * config->height;
    return area * 100 < screen * config->auto_ui_percent ? INK_DISPLAY_INTENT_UI : INK_DISPLAY_INTENT_TEXT;
}

/* ---- Decisions: built only here, so the action and its field always match ---- */

static ink_display_policy_decision_t decision_none(void) {
    return (ink_display_policy_decision_t){.action = INK_DISPLAY_ACTION_NONE};
}

static ink_display_policy_decision_t decision_draw(ink_display_intent_t intent, ink_display_rect_t window) {
    return (ink_display_policy_decision_t){
        .action    = INK_DISPLAY_ACTION_DRAW,
        .intent    = intent,
        .window    = window,
        .old_plane = INK_DISPLAY_POLICY_OLD_SHOWN,
    };
}

static ink_display_policy_decision_t decision_cleanup(ink_display_cleanup_t cleanup, ink_display_rect_t window) {
    return (ink_display_policy_decision_t){
        .action    = INK_DISPLAY_ACTION_CLEANUP,
        .cleanup   = cleanup,
        .window    = window,
        .old_plane = INK_DISPLAY_POLICY_OLD_INVERTED,
    };
}

/* Whole-panel cleanup (STRONG or FAST) with an inverted old plane; all debt and edge flags back to 0. */
static void decide_clean(ink_display_policy_state_t *state, ink_display_cleanup_t cleanup, ink_display_policy_decision_t *out_decision) {
    clear_pixels(state);
    state->priv.shown_valid = true;
    state->priv.after_wake  = false;
    *out_decision           = decision_cleanup(cleanup, whole_screen(&state->priv.config));
}

static bool has_dirt(const ink_display_policy_state_t *state) {
    return state->priv.max_debt > 0 || state->priv.flagged_count > 0;
}

ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state, const uint8_t *shown, const uint8_t *next,
                                                     ink_display_policy_request_t request, uint32_t now_ms,
                                                     ink_display_policy_decision_t *out_decision) {
    if (state == nullptr || next == nullptr || out_decision == nullptr || !request_valid(request)) {
        return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
    }
    if (state->priv.shown_valid && shown == nullptr) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    const ink_display_policy_config_t *config = &state->priv.config;

    const bool before_sleep = (request.flags & INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP) != 0;
    if (!state->priv.shown_valid || (before_sleep && has_dirt(state))) {
        (void)start_idle_period(state, request, now_ms);
        decide_clean(state, INK_DISPLAY_CLEANUP_STRONG, out_decision);
        return INK_DISPLAY_POLICY_OK;
    }
    bounds_t changed;
    if (!find_changed_bounds(config, shown, next, &changed)) {
        state->priv.defer = (request.flags & INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP) != 0;
        *out_decision     = decision_none();
        return INK_DISPLAY_POLICY_OK;
    }
    const bool active = start_idle_period(state, request, now_ms);

    const bool                 after_wake = state->priv.after_wake;
    const ink_display_rect_t   window     = after_wake ? whole_screen(config) : make_window(config, changed);
    const ink_display_intent_t intent     = resolve_intent(config, request.intent, window);
    const uint32_t             weight     = config->debt_weights[intent];
    state->priv.after_wake                = false;

    if (!after_wake && would_exceed(state, shown, next, changed, weight, state->priv.ceiling)) {
        decide_clean(state, active ? INK_DISPLAY_CLEANUP_FAST : INK_DISPLAY_CLEANUP_STRONG, out_decision);
        return INK_DISPLAY_POLICY_OK;
    }
    if (intent == INK_DISPLAY_INTENT_PAGE && state->priv.max_debt >= config->debt_page) {
        decide_clean(state, INK_DISPLAY_CLEANUP_STRONG, out_decision);
        return INK_DISPLAY_POLICY_OK;
    }
    add_debt(state, shown, next, changed, weight);
    if (config->marks_edges[intent]) { mark_edges(state, shown, next, changed); }

    *out_decision = decision_draw(intent, window);
    return INK_DISPLAY_POLICY_OK;
}

/* ---- Idle cleanup stages ---- */

typedef struct {
    ink_display_cleanup_t cleanup;
    uint32_t              after_ms;
} stage_t;

/* Stages not yet run that still have work, in order; returns the count. */
static size_t pending_stages(const ink_display_policy_state_t *state, stage_t out_stages[3]) {
    const ink_display_policy_config_t *config = &state->priv.config;
    const uint8_t                      done   = state->priv.stages_done;
    size_t                             count  = 0;
    if (config->edge_ms > 0 && (done & STAGE_EDGE) == 0 && state->priv.flagged_count > 0) {
        const ink_display_cleanup_t cleanup =
            state->priv.edge_center_done ? INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR : INK_DISPLAY_CLEANUP_EDGE_CENTER;
        out_stages[count++] = (stage_t){.cleanup = cleanup, .after_ms = config->edge_ms};
    }
    if (config->light_ms > 0 && (done & STAGE_LIGHT) == 0 && state->priv.max_debt >= config->debt_light) {
        out_stages[count++] = (stage_t){.cleanup = INK_DISPLAY_CLEANUP_LIGHT, .after_ms = config->light_ms};
    }
    if (config->strong_ms > 0 && (done & STAGE_STRONG) == 0 && has_dirt(state)) {
        out_stages[count++] = (stage_t){.cleanup = INK_DISPLAY_CLEANUP_STRONG, .after_ms = config->strong_ms};
    }
    return count;
}

ink_display_policy_error_t ink_display_policy_idle(const ink_display_policy_state_t *state, uint32_t now_ms,
                                                   ink_display_policy_idle_t *out_idle) {
    if (state == nullptr || out_idle == nullptr) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    *out_idle = (ink_display_policy_idle_t){.action = INK_DISPLAY_ACTION_NONE, .wait_ms = INK_DISPLAY_POLICY_WAIT_FOREVER};
    if (!state->priv.shown_valid || state->priv.after_wake || !state->priv.has_frame || state->priv.defer) { return INK_DISPLAY_POLICY_OK; }

    stage_t        stages[3];
    const size_t   count   = pending_stages(state, stages);
    const uint32_t elapsed = elapsed_ms(state->priv.last_frame_ms, now_ms);
    for (size_t i = 0; i < count; i++) {
        if (elapsed >= stages[i].after_ms) {
            out_idle->action  = INK_DISPLAY_ACTION_CLEANUP;
            out_idle->cleanup = stages[i].cleanup;
            return INK_DISPLAY_POLICY_OK;
        }
        const uint32_t wait_ms = stages[i].after_ms - elapsed;
        if (wait_ms < out_idle->wait_ms) { out_idle->wait_ms = wait_ms; }
    }
    return INK_DISPLAY_POLICY_OK;
}

/* Cleanups that idle() returns; FAST comes only from decide(). */
static bool idle_cleanup(ink_display_cleanup_t cleanup) {
    return cleanup == INK_DISPLAY_CLEANUP_EDGE_CENTER
        || cleanup == INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR
        || cleanup == INK_DISPLAY_CLEANUP_LIGHT
        || cleanup == INK_DISPLAY_CLEANUP_STRONG;
}

/* White pixels next to a dark-edge center, black pixels next to a light-edge center (US 11,568,827, col. 12). */
static bool near_matching_center(const ink_display_policy_state_t *state, const uint8_t *shown, uint32_t x, uint32_t y) {
    const ink_display_policy_config_t *config = &state->priv.config;
    const uint8_t                      want   = is_white(config, shown, x, y) ? FLAG_DARK : FLAG_LIGHT;
    for (size_t i = 0; i < 4; i++) {
        uint32_t nx, ny;
        if (neighbor(config, x, y, s_cardinal[i][0], s_cardinal[i][1], &nx, &ny) && flag_of(*pixel_state(state, nx, ny)) == want) {
            return true;
        }
    }
    return false;
}

static bool selected_for(const ink_display_policy_state_t *state, const uint8_t *shown, ink_display_cleanup_t cleanup, uint32_t x,
                         uint32_t y) {
    const uint8_t pixel = *pixel_state(state, x, y);
    switch (cleanup) {
        case INK_DISPLAY_CLEANUP_EDGE_CENTER:
            return flag_of(pixel) != FLAG_NONE;
        case INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR:
            return flag_of(pixel) == FLAG_NONE && near_matching_center(state, shown, x, y);
        case INK_DISPLAY_CLEANUP_LIGHT:
            return debt_of(pixel) >= state->priv.config.debt_light;
        default:
            return true; /* CLEANUP_STRONG */
    }
}

ink_display_policy_error_t ink_display_policy_cleanup_plane(const ink_display_policy_state_t *state, const uint8_t *shown,
                                                            ink_display_cleanup_t cleanup, uint8_t *out_old,
                                                            ink_display_rect_t *out_window) {
    if (state == nullptr || shown == nullptr || out_old == nullptr || out_window == nullptr || !idle_cleanup(cleanup)) {
        return INK_DISPLAY_POLICY_ERROR_ARGUMENT;
    }
    const ink_display_policy_config_t *config = &state->priv.config;
    memcpy(out_old, shown, row_bytes(config) * config->height);
    bounds_t bounds = empty_bounds();
    for (uint32_t y = 0; y < config->height; y++) {
        for (uint32_t x = 0; x < config->width; x++) {
            if (!selected_for(state, shown, cleanup, x, y)) { continue; }
            invert_pixel(config, out_old, x, y);
            add_to_bounds(&bounds, x, y);
        }
    }
    *out_window = bounds_empty(&bounds) ? (ink_display_rect_t){} : make_window(config, bounds);
    if (cleanup == INK_DISPLAY_CLEANUP_STRONG) { *out_window = whole_screen(config); }
    return INK_DISPLAY_POLICY_OK;
}

ink_display_policy_error_t ink_display_policy_cleanup_done(ink_display_policy_state_t *state, ink_display_cleanup_t cleanup) {
    if (state == nullptr || !idle_cleanup(cleanup)) { return INK_DISPLAY_POLICY_ERROR_ARGUMENT; }
    const size_t count = pixel_count(&state->priv.config);
    switch (cleanup) {
        case INK_DISPLAY_CLEANUP_EDGE_CENTER:
            state->priv.edge_center_done = true;
            break;
        case INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR:
            for (size_t i = 0; i < count; i++) { state->priv.pixel_state[i] &= (uint8_t)~FLAG_MASK; }
            state->priv.flagged_count = 0;
            state->priv.stages_done |= STAGE_EDGE;
            break;
        case INK_DISPLAY_CLEANUP_LIGHT:
            for (size_t i = 0; i < count; i++) {
                if (debt_of(state->priv.pixel_state[i]) >= state->priv.config.debt_light) { set_debt(&state->priv.pixel_state[i], 0); }
            }
            recompute_max_debt(state);
            state->priv.stages_done |= STAGE_LIGHT;
            break;
        default: /* CLEANUP_STRONG */
            clear_pixels(state);
            state->priv.stages_done |= STAGE_EDGE | STAGE_LIGHT | STAGE_STRONG;
            break;
    }
    return INK_DISPLAY_POLICY_OK;
}
