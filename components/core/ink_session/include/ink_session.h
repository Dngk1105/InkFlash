/* Design: docs/design/wake-session.md */
#pragma once

/*
 * State machine of one wake session: boot -> restore -> active -> save -> deep sleep. Takes events (boot, button,
 * idle timer, keep awake, sleep request, display power off), returns the action system must run. Includes nothing
 * from ESP-IDF or FreeRTOS and never reads a clock: the caller passes the time in every event.
 *
 * Also checks the header of an RTC slot (magic, version, payload length, CRC); the CRC function comes from the caller
 * so host tests run without the ROM.
 */
#include <stddef.h>
#include <stdint.h>

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_SESSION_OK = 0,
    INK_SESSION_ERROR_ARGUMENT,
} ink_session_error_t;

/* ---- Phases ---- */

/** Phase of the wake session; one row of the state table in docs/design/wake-session.md. */
typedef enum : uint8_t {
    INK_SESSION_PHASE_BOOT = 0, /**< chip just booted; waits for BOOT */
    INK_SESSION_PHASE_RESTORE,  /**< snapshots being read (or cold boot); waits for RESTORED */
    INK_SESSION_PHASE_ACTIVE,   /**< in use; the idle timer runs */
    INK_SESSION_PHASE_SAVE,     /**< snapshots written, background frame drawing; waits for POWER_OFF */
    INK_SESSION_PHASE_SLEEP,    /**< deep sleep asked; no event changes anything */
} ink_session_phase_t;

/* ---- Events in, actions out ---- */

typedef enum : uint8_t {
    INK_SESSION_EVENT_BOOT = 0,   /**< `value` = 1 when the reset reason is deep sleep wake, 0 for cold boot */
    INK_SESSION_EVENT_RESTORED,   /**< snapshots read (or cold boot handled); the first screen is on its way */
    INK_SESSION_EVENT_ACTIVITY,   /**< a button was pressed or released */
    INK_SESSION_EVENT_KEEP_AWAKE, /**< app asks to stay awake `value` ms from `time_ms`; capped at INK_SESSION_KEEP_AWAKE_MAX_MS */
    INK_SESSION_EVENT_TIMER,      /**< the idle timer fired */
    INK_SESSION_EVENT_SLEEP,      /**< sleep asked now: HOLD_AB_LONG or the Sleep item in Home */
    INK_SESSION_EVENT_POWER_OFF,  /**< display reported analog power off */
    INK_SESSION_EVENT_COUNT,
} ink_session_event_type_t;

typedef struct {
    ink_session_event_type_t type;
    uint32_t                 time_ms; /**< caller's clock; wraps after 49 days, the block compares by difference */
    uint32_t                 value;   /**< meaning depends on `type` */
} ink_session_event_t;

typedef enum : uint8_t {
    INK_SESSION_ACTION_NONE = 0,
    INK_SESSION_ACTION_RESTORE,    /**< deep sleep wake: read RTC slots, rebuild the background frame, then send RESTORED */
    INK_SESSION_ACTION_COLD_BOOT,  /**< ignore RTC slots, open Home, then send RESTORED */
    INK_SESSION_ACTION_ARM_TIMER,  /**< (re)start the idle timer for `delay_ms` */
    INK_SESSION_ACTION_SAVE,       /**< stop the idle timer, write snapshots, draw the background frame with BEFORE_SLEEP */
    INK_SESSION_ACTION_DEEP_SLEEP, /**< enter deep sleep */
} ink_session_action_t;

typedef struct {
    ink_session_action_t action;
    uint32_t             delay_ms; /**< ARM_TIMER only */
} ink_session_output_t;

#define INK_SESSION_KEEP_AWAKE_MAX_MS 30000u /**< longest stay-awake of one KEEP_AWAKE event */

/* ---- State ---- */

/** The caller owns the struct but must not touch its fields. */
typedef struct {
    struct {
        ink_session_phase_t phase;
        uint32_t            idle_ms;     /**< no activity for this long -> save and sleep */
        uint32_t            activity_ms; /**< time of the last activity (or of RESTORED) */
        uint32_t            keep_until;  /**< end of the latest keep awake; valid when has_keep */
        bool                has_keep;
    } priv;
} ink_session_state_t;

/* ---- API ---- */

/**
 * @brief Reset the state to BOOT.
 * @par Called from
 * Main task, once at boot.
 * @retval INK_SESSION_ERROR_ARGUMENT state is NULL or idle_ms is 0.
 */
[[nodiscard]] ink_session_error_t ink_session_init(ink_session_state_t *state, uint32_t idle_ms);

/**
 * @brief Handle one event; write the action to run into `out`. An event that does not fit the phase gives NONE.
 * @par Called from
 * Main task, once per event.
 * @retval INK_SESSION_ERROR_ARGUMENT a pointer is NULL or the event type is invalid.
 */
[[nodiscard]] ink_session_error_t ink_session_handle(ink_session_state_t *state, const ink_session_event_t *event,
                                                     ink_session_output_t *out);

/**
 * @brief Current phase; BOOT if state is NULL.
 * @par Called from
 * Main task.
 */
ink_session_phase_t ink_session_phase(const ink_session_state_t *state);

/* ---- RTC slot header ---- */

/** Header at the start of every RTC slot; the payload follows it. */
typedef struct {
    uint32_t magic;   /**< INK_SESSION_SLOT_MAGIC */
    uint16_t version; /**< layout version of the payload, chosen by the owner module */
    uint16_t length;  /**< payload bytes */
    uint32_t crc;     /**< CRC of the payload */
} ink_session_slot_header_t;

#define INK_SESSION_SLOT_MAGIC 0x534B4E49u /**< "INKS" in little endian */

/** CRC of `length` bytes; system passes the ROM CRC32, tests pass their own. */
typedef uint32_t (*ink_session_crc_t)(const uint8_t *data, size_t length);

/**
 * @brief Payload area of a slot: the bytes right after the header.
 * @return Payload start; NULL if slot is NULL or slot_size is not larger than the header.
 */
uint8_t *ink_session_slot_payload(uint8_t *slot, size_t slot_size);

/**
 * @brief Write the header for a payload already written into the slot.
 * @par Called from
 * Main task, before deep sleep.
 * @retval INK_SESSION_ERROR_ARGUMENT a pointer is NULL or the payload does not fit the slot.
 */
[[nodiscard]] ink_session_error_t ink_session_slot_seal(uint8_t *slot, size_t slot_size, uint16_t version, size_t length,
                                                        ink_session_crc_t crc);

/**
 * @brief Check the header: magic, version, length that fits the slot, CRC.
 * @par Called from
 * Main task, after deep sleep wake.
 * @param out_length Payload bytes when the slot is valid; 0 otherwise. May be NULL.
 * @return true when the slot can be read; false means the owner starts fresh.
 */
bool ink_session_slot_check(const uint8_t *slot, size_t slot_size, uint16_t version, ink_session_crc_t crc, size_t *out_length);
