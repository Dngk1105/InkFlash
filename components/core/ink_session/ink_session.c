/* Design: docs/design/wake-session.md */
#include "ink_session.h"

#include <string.h>

ink_session_error_t ink_session_init(ink_session_state_t *state, uint32_t idle_ms) {
    if (state == nullptr || idle_ms == 0) { return INK_SESSION_ERROR_ARGUMENT; }
    state->priv = (typeof(state->priv)){.phase = INK_SESSION_PHASE_BOOT, .idle_ms = idle_ms};
    return INK_SESSION_OK;
}

ink_session_phase_t ink_session_phase(const ink_session_state_t *state) {
    return state != nullptr ? state->priv.phase : INK_SESSION_PHASE_BOOT;
}

/* true when time a is later than time b; works across the 32-bit wrap. */
static bool is_after(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) > 0;
}

/* Time the machine may sleep: idle_ms after the last activity, or the end of keep awake if later. */
static uint32_t sleep_deadline(const ink_session_state_t *state) {
    const uint32_t idle_end = state->priv.activity_ms + state->priv.idle_ms;
    if (state->priv.has_keep && is_after(state->priv.keep_until, idle_end)) { return state->priv.keep_until; }
    return idle_end;
}

/* Arm the timer for the rest of the wait; time already past the deadline means save now. */
static void arm_or_save(ink_session_state_t *state, uint32_t now_ms, ink_session_output_t *out) {
    const uint32_t deadline = sleep_deadline(state);
    if (!is_after(deadline, now_ms)) {
        state->priv.phase = INK_SESSION_PHASE_SAVE;
        out->action       = INK_SESSION_ACTION_SAVE;
        return;
    }
    out->action   = INK_SESSION_ACTION_ARM_TIMER;
    out->delay_ms = deadline - now_ms;
}

ink_session_error_t ink_session_handle(ink_session_state_t *state, const ink_session_event_t *event, ink_session_output_t *out) {
    if (state == nullptr || event == nullptr || out == nullptr) { return INK_SESSION_ERROR_ARGUMENT; }
    if (event->type >= INK_SESSION_EVENT_COUNT) { return INK_SESSION_ERROR_ARGUMENT; }
    *out                            = (ink_session_output_t){.action = INK_SESSION_ACTION_NONE};
    const ink_session_phase_t phase = state->priv.phase;
    const uint32_t            now   = event->time_ms;
    switch (event->type) {
        case INK_SESSION_EVENT_BOOT:
            if (phase != INK_SESSION_PHASE_BOOT) { break; }
            state->priv.phase = INK_SESSION_PHASE_RESTORE;
            out->action       = event->value != 0 ? INK_SESSION_ACTION_RESTORE : INK_SESSION_ACTION_COLD_BOOT;
            break;
        case INK_SESSION_EVENT_RESTORED:
            if (phase != INK_SESSION_PHASE_RESTORE) { break; }
            state->priv.phase       = INK_SESSION_PHASE_ACTIVE;
            state->priv.activity_ms = now;
            arm_or_save(state, now, out);
            break;
        case INK_SESSION_EVENT_ACTIVITY:
            if (phase != INK_SESSION_PHASE_ACTIVE) { break; }
            state->priv.activity_ms = now;
            arm_or_save(state, now, out);
            break;
        case INK_SESSION_EVENT_KEEP_AWAKE: {
            if (phase != INK_SESSION_PHASE_ACTIVE) { break; }
            const uint32_t ms  = event->value < INK_SESSION_KEEP_AWAKE_MAX_MS ? event->value : INK_SESSION_KEEP_AWAKE_MAX_MS;
            const uint32_t end = now + ms;
            if (!state->priv.has_keep || is_after(end, state->priv.keep_until)) {
                state->priv.keep_until = end;
                state->priv.has_keep   = true;
            }
            arm_or_save(state, now, out);
            break;
        }
        case INK_SESSION_EVENT_TIMER:
            if (phase != INK_SESSION_PHASE_ACTIVE) { break; } /* a timer that fired just before SLEEP: already saving */
            arm_or_save(state, now, out);
            break;
        case INK_SESSION_EVENT_SLEEP:
            if (phase != INK_SESSION_PHASE_ACTIVE) { break; }
            state->priv.phase = INK_SESSION_PHASE_SAVE;
            out->action       = INK_SESSION_ACTION_SAVE;
            break;
        case INK_SESSION_EVENT_POWER_OFF:
            /* While ACTIVE, display also powers off after power_off_ms without a draw; only the one after SAVE counts. */
            if (phase != INK_SESSION_PHASE_SAVE) { break; }
            state->priv.phase = INK_SESSION_PHASE_SLEEP;
            out->action       = INK_SESSION_ACTION_DEEP_SLEEP;
            break;
        case INK_SESSION_EVENT_COUNT:
            return INK_SESSION_ERROR_ARGUMENT;
    }
    return INK_SESSION_OK;
}

/* ---- RTC slot header ---- */

uint8_t *ink_session_slot_payload(uint8_t *slot, size_t slot_size) {
    if (slot == nullptr || slot_size <= sizeof(ink_session_slot_header_t)) { return nullptr; }
    return slot + sizeof(ink_session_slot_header_t);
}

ink_session_error_t ink_session_slot_seal(uint8_t *slot, size_t slot_size, uint16_t version, size_t length, ink_session_crc_t crc) {
    uint8_t *payload = ink_session_slot_payload(slot, slot_size);
    if (payload == nullptr || crc == nullptr) { return INK_SESSION_ERROR_ARGUMENT; }
    if (length > slot_size - sizeof(ink_session_slot_header_t) || length > UINT16_MAX) { return INK_SESSION_ERROR_ARGUMENT; }
    const ink_session_slot_header_t header = {
        .magic   = INK_SESSION_SLOT_MAGIC,
        .version = version,
        .length  = (uint16_t)length,
        .crc     = crc(payload, length),
    };
    memcpy(slot, &header, sizeof(header)); /* memcpy: the slot start may not be aligned for the header */
    return INK_SESSION_OK;
}

bool ink_session_slot_check(const uint8_t *slot, size_t slot_size, uint16_t version, ink_session_crc_t crc, size_t *out_length) {
    if (out_length != nullptr) { *out_length = 0; }
    if (slot == nullptr || crc == nullptr || slot_size <= sizeof(ink_session_slot_header_t)) { return false; }
    ink_session_slot_header_t header;
    memcpy(&header, slot, sizeof(header));
    if (header.magic != INK_SESSION_SLOT_MAGIC || header.version != version) { return false; }
    if (header.length > slot_size - sizeof(header)) { return false; }
    if (crc(slot + sizeof(header), header.length) != header.crc) { return false; }
    if (out_length != nullptr) { *out_length = header.length; }
    return true;
}
