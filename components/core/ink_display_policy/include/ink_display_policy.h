/* Design: docs/design/display/display.md */
#pragma once

/*
 * Decision side of the display: compare the new frame with the shown frame, find the changed window, keep debt
 * and edge flags per pixel, pick the decision, schedule cleanup stages while idle, build the old plane for a
 * cleanup. Pure C (C23). Frame layout follows the display HAL: 1 bit per pixel (1 = white), row by row, leftmost
 * pixel in the most significant bit; each row is (width + 7) / 8 bytes. The caller provides pixel_state memory:
 * width × height bytes.
 */
#include <stddef.h>
#include <stdint.h>

#define INK_DISPLAY_POLICY_MAX_DEBT     15
#define INK_DISPLAY_POLICY_WAIT_FOREVER UINT32_MAX /**< no cleanup stage left to wait for */

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OK = 0,
    INK_DISPLAY_POLICY_ERROR_ARGUMENT,
} ink_display_policy_error_t;

/* ---- Types ---- */

/** Intent of a frame.
 * WARN: append only; the app and the bench firmware send these values as numbers. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_INTENT_AUTO,
    INK_DISPLAY_POLICY_INTENT_TEXT,
    INK_DISPLAY_POLICY_INTENT_UI,
    INK_DISPLAY_POLICY_INTENT_PAGE,
    INK_DISPLAY_POLICY_INTENT_COUNT,
} ink_display_policy_intent_t;

/** Flags submitted with a frame. */
enum : uint8_t {
    INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP = 1 << 0, /**< no cleanup stage until a frame without this flag */
    INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP  = 1 << 1, /**< background frame before sleep: any debt left -> cleanup strong */
};

/** What to do with the panel.
 * WARN: append only; the bench firmware sends these values as numbers. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_ACTION_NONE,
    INK_DISPLAY_POLICY_ACTION_DRAW, /**< preset of the intent, window = changed window */
    INK_DISPLAY_POLICY_ACTION_CLEANUP_STRONG,
    INK_DISPLAY_POLICY_ACTION_CLEANUP_FAST,
    INK_DISPLAY_POLICY_ACTION_CLEANUP_EDGE_CENTER,
    INK_DISPLAY_POLICY_ACTION_CLEANUP_EDGE_NEIGHBOR,
    INK_DISPLAY_POLICY_ACTION_CLEANUP_LIGHT,
} ink_display_policy_action_t;

/** Old plane of a draw. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OLD_SHOWN,    /**< shown frame */
    INK_DISPLAY_POLICY_OLD_INVERTED, /**< new frame inverted: every pixel is driven */
} ink_display_policy_old_plane_t;

/** Coordinates in native panel orientation, px. */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_display_policy_rect_t;

typedef struct {
    uint16_t width; /**< native panel width, px, from driver capabilities */
    uint16_t height;
    uint8_t  x_align;                                       /**< window x edges are multiples of this */
    uint8_t  debt_weights[INK_DISPLAY_POLICY_INTENT_COUNT]; /**< debt added per changed pixel; index AUTO unused */
    bool     marks_edges[INK_DISPLAY_POLICY_INTENT_COUNT];  /**< frames with this intent set edge flags; index AUTO unused */
    uint8_t  auto_ui_percent;                               /**< AUTO: window smaller than this % of the panel area -> UI */
    uint8_t  debt_light;                                    /**< cleanup light covers pixels with debt >= this */
    uint8_t  debt_page;                                     /**< PAGE with max debt >= this -> cleanup strong */
    uint8_t  debt_ceiling;                                  /**< debt ceiling after idle */
    uint8_t  debt_ceiling_max;                              /**< highest debt ceiling during an active period */
    uint32_t active_ms; /**< frame arriving within this time after the previous one = active period: ceiling +1; 0 = off */
    uint32_t edge_ms;   /**< cleanup stage 1 delay; 0 = skip */
    uint32_t light_ms;  /**< cleanup stage 2 delay; 0 = skip */
    uint32_t strong_ms; /**< cleanup stage 3 delay; 0 = skip */
} ink_display_policy_config_t;

typedef struct {
    ink_display_policy_intent_t intent;
    uint8_t                     flags; /**< INK_DISPLAY_POLICY_FLAG_* */
} ink_display_policy_request_t;

typedef struct {
    ink_display_policy_action_t    action; /**< NONE, DRAW, CLEANUP_STRONG or CLEANUP_FAST */
    ink_display_policy_intent_t    intent; /**< DRAW: resolved intent (TEXT, UI, PAGE) */
    ink_display_policy_rect_t      window; /**< whole panel for a cleanup; size 0 for NONE */
    ink_display_policy_old_plane_t old_plane;
} ink_display_policy_decision_t;

typedef struct {
    ink_display_policy_action_t action;  /**< NONE: wait wait_ms, then ask again */
    uint32_t                    wait_ms; /**< INK_DISPLAY_POLICY_WAIT_FOREVER when no stage is left */
} ink_display_policy_idle_t;

typedef struct {
    struct {
        ink_display_policy_config_t config;
        uint8_t                    *pixel_state; /**< per pixel: bits 0-1 edge flags, bits 2-5 debt */
        uint32_t                    flagged_count;
        uint32_t                    last_frame_ms;
        uint8_t                     max_debt;
        uint8_t                     ceiling;
        uint8_t                     stages_done; /**< one bit per cleanup stage done in this idle period */
        bool                        edge_center_done;
        bool                        defer;
        bool                        has_frame;   /**< last_frame_ms is valid */
        bool                        shown_valid; /**< the panel shows exactly the caller's `shown` frame */
        bool                        after_wake;  /**< `shown` is the background frame drawn before sleep */
    } priv;
} ink_display_policy_state_t;

/* ---- API ---- */

/**
 * @brief Reset: every pixel has debt 0 and no edge flag; the first decide is cleanup strong.
 * @par Called from
 * Display, at boot.
 * @param pixel_state width × height bytes; must live as long as state.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL or config is invalid.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t        *state,
                                                                 const ink_display_policy_config_t *config, uint8_t *pixel_state);

/**
 * @brief Change config for later calls; keep debt and edge flags. Panel size and x_align must not change.
 * @par Called from
 * Display task.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL, config is invalid or changes the panel size;
 *         the old config stays.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_set_config(ink_display_policy_state_t        *state,
                                                                       const ink_display_policy_config_t *config);

/**
 * @brief Pick the decision for `next` and assume the panel shows `next` afterwards; update debt and edge flags,
 *        restart the cleanup stages.
 * @par Called from
 * Display task, once per frame.
 * @param shown Shown frame; NULL when there is none (first frame, or after invalidate).
 * @param now_ms Caller's ms clock; may wrap.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL, intent is out of range, or shown is NULL while
 *         the panel has a shown frame.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state, const uint8_t *shown,
                                                                   const uint8_t *next, ink_display_policy_request_t request,
                                                                   uint32_t now_ms, ink_display_policy_decision_t *out_decision);

/**
 * @brief Next cleanup stage while no new frame arrives.
 * @par Called from
 * Display task, after each frame and after each cleanup.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_idle(const ink_display_policy_state_t *state, uint32_t now_ms,
                                                                 ink_display_policy_idle_t *out_idle);

/**
 * @brief Build the old plane for cleanup `action` (CLEANUP_EDGE_CENTER, CLEANUP_EDGE_NEIGHBOR, CLEANUP_LIGHT,
 *        CLEANUP_STRONG): copy `shown`, then invert the pixels being cleaned (cleanup strong: every pixel).
 *        The new plane of the cleanup is `shown`.
 * @par Called from
 * Display task, after idle.
 * @param out_window Bounds of the cleaned pixels, x edges aligned; size 0 if there are none.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL or action is not an idle cleanup.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_cleanup_plane(const ink_display_policy_state_t *state, const uint8_t *shown,
                                                                          ink_display_policy_action_t action, uint8_t *out_old,
                                                                          ink_display_policy_rect_t *out_window);

/**
 * @brief Cleanup `action` finished (or had no pixels): clear the cleaned edge flags and debt, mark the stage done.
 * @par Called from
 * Display task.
 * @retval INK_DISPLAY_POLICY_ERROR_ARGUMENT a pointer is NULL or action is not an idle cleanup.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_cleanup_done(ink_display_policy_state_t *state,
                                                                         ink_display_policy_action_t action);

/**
 * @brief Panel content is unknown (driver draw failed): the next decide is cleanup strong. Ignores NULL state.
 * @par Called from
 * Display task.
 */
void ink_display_policy_invalidate(ink_display_policy_state_t *state);

/**
 * @brief After wake, the caller's `shown` is the background frame drawn before sleep: the next decide draws the
 *        whole panel with the preset of the intent. Ignores NULL state.
 * @par Called from
 * Display, at boot.
 */
void ink_display_policy_assume_shown(ink_display_policy_state_t *state);

/**
 * @brief Highest debt over all pixels.
 * @par Called from
 * Display task.
 * @return Max debt; 0 if state is NULL.
 */
uint8_t ink_display_policy_max_debt(const ink_display_policy_state_t *state);

/**
 * @brief Number of pixels with an edge flag.
 * @par Called from
 * Display task.
 * @return Pixel count; 0 if state is NULL.
 */
uint32_t ink_display_policy_flagged_count(const ink_display_policy_state_t *state);

/**
 * @brief Debt ceiling in effect.
 * @par Called from
 * Display task.
 * @return Ceiling; 0 if state is NULL.
 */
uint8_t ink_display_policy_ceiling(const ink_display_policy_state_t *state);

/**
 * @brief Rect bounding the pixels that differ between two frames; x edges aligned to config->x_align.
 * @par Called from
 * Any task.
 * @retval false a pointer is NULL or the frames are identical.
 */
bool ink_display_policy_changed_window(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next,
                                       ink_display_policy_rect_t *out_window);
