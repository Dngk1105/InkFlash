/* Design: docs/design/display/display.md */
#pragma once

/*
 * Owner of the 3 display frames. Tracks only who owns each frame (by index), not the image memory.
 * No internal lock: the caller holds a lock around every call, because the main task and the display
 * task both call it. Pure C (C23).
 */
#include <stdint.h>

#define INK_FRAME_SLOTS_COUNT 3
#define INK_FRAME_SLOTS_NONE  (-1) /**< no frame */

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_FRAME_SLOTS_OK = 0,
    INK_FRAME_SLOTS_ERROR_ARGUMENT,
    INK_FRAME_SLOTS_ERROR_STATE, /**< wrong call order: acquire twice, submit without acquire... */
} ink_frame_slots_error_t;

/* ---- Types ---- */

/** What the caller must do with the image in the frame it just acquired. */
typedef enum : uint8_t {
    INK_FRAME_SLOTS_FILL_WHITE, /**< no frame submitted yet: fill white */
    INK_FRAME_SLOTS_FILL_COPY,  /**< copy the image from frame `copy_from` */
    INK_FRAME_SLOTS_FILL_KEEP,  /**< frame taken back from pending; already holds the latest image */
} ink_frame_slots_fill_t;

/** Result of one acquire. */
typedef struct {
    int8_t                 frame; /**< index of the acquired frame */
    ink_frame_slots_fill_t fill;
    int8_t                 copy_from; /**< latest submitted frame when fill = COPY; NONE otherwise */
} ink_frame_slots_lease_t;

/** The caller owns the struct but must not touch `priv`. */
typedef struct {
    struct {
        uint8_t  owner[INK_FRAME_SLOTS_COUNT];
        int8_t   acquired; /* frame the main task is drawing */
        int8_t   pending;  /* submitted, not yet taken by the display task */
        int8_t   latest;   /* latest submitted frame; the next acquire copies from it */
        int8_t   shown;    /* frame on the panel */
        uint32_t dropped;
    } priv;
} ink_frame_slots_state_t;

/* ---- API ---- */

/**
 * @brief Mark every frame free; no shown frame.
 * @par Called from
 * Boot, before the display task starts.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT state is NULL.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_init(ink_frame_slots_state_t *state);

/**
 * @brief Make `frame` the shown frame and the latest frame (background frame after wake).
 * @par Called from
 * Boot, after init.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT state is NULL, or frame is outside 0..COUNT-1.
 * @retval INK_FRAME_SLOTS_ERROR_STATE frame is not free.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_assume_shown(ink_frame_slots_state_t *state, int8_t frame);

/**
 * @brief Acquire a frame to draw into.
 *
 * No free frame (the display task holds two): take back the pending frame; it counts as dropped.
 * @par Called from
 * Main task.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT a pointer is NULL.
 * @retval INK_FRAME_SLOTS_ERROR_STATE the previous frame is acquired and not submitted.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_acquire(ink_frame_slots_state_t *state, ink_frame_slots_lease_t *out_lease);

/**
 * @brief Frame the main task has acquired.
 * @par Called from
 * Main task.
 * @return Frame index, or INK_FRAME_SLOTS_NONE.
 */
int8_t ink_frame_slots_acquired(const ink_frame_slots_state_t *state);

/**
 * @brief Submit the acquired frame as pending. A frame already pending is dropped.
 * @par Called from
 * Main task.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT state is NULL.
 * @retval INK_FRAME_SLOTS_ERROR_STATE no frame is acquired.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_submit(ink_frame_slots_state_t *state);

/**
 * @brief Take the pending frame to draw it.
 * @par Called from
 * Display task.
 * @return Frame index, or INK_FRAME_SLOTS_NONE if nothing is pending or state is NULL.
 */
int8_t ink_frame_slots_take(ink_frame_slots_state_t *state);

/**
 * @brief Make the taken frame the shown frame; the old shown frame becomes free.
 * @par Called from
 * Display task, after the draw finishes (also after a failed draw) or when no draw is needed.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT state is NULL, or frame is outside 0..COUNT-1.
 * @retval INK_FRAME_SLOTS_ERROR_STATE frame is not the taken frame.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_show(ink_frame_slots_state_t *state, int8_t frame);

/**
 * @brief Free the taken frame without showing it (policy error).
 * @par Called from
 * Display task.
 * @retval INK_FRAME_SLOTS_ERROR_ARGUMENT state is NULL, or frame is outside 0..COUNT-1.
 * @retval INK_FRAME_SLOTS_ERROR_STATE frame is not the taken frame.
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_release(ink_frame_slots_state_t *state, int8_t frame);

/**
 * @brief Shown frame.
 * @par Called from
 * Any task, under the caller's lock.
 * @return Frame index, or INK_FRAME_SLOTS_NONE.
 */
int8_t ink_frame_slots_shown(const ink_frame_slots_state_t *state);

/**
 * @brief Frames dropped since init (overwritten while pending, or taken back by acquire).
 * @par Called from
 * Any task, under the caller's lock.
 * @return Drop count; 0 if state is NULL.
 */
uint32_t ink_frame_slots_dropped(const ink_frame_slots_state_t *state);
