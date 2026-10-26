/* Thiết kế: docs/design/code-structure.md */
#include "ink_example.h"

ink_example_error_t ink_example_init(ink_example_state_t *state, uint32_t limit)
{
    if (state == nullptr || limit == 0) { return INK_EXAMPLE_ERROR_ARGUMENT; }
    state->priv.count = 0;
    state->priv.limit = limit;
    return INK_EXAMPLE_OK;
}

ink_example_error_t ink_example_handle(ink_example_state_t *state, const ink_example_event_t *event,
                                       ink_example_action_t *out_action)
{
    if (state == nullptr || event == nullptr || out_action == nullptr) { return INK_EXAMPLE_ERROR_ARGUMENT; }
    *out_action = INK_EXAMPLE_ACTION_NONE;
    switch (event->type) {
        case INK_EXAMPLE_EVENT_TICK:
            state->priv.count++;
            if (state->priv.count >= state->priv.limit) {
                state->priv.count = 0;
                *out_action       = INK_EXAMPLE_ACTION_NOTIFY;
            }
            return INK_EXAMPLE_OK;
        case INK_EXAMPLE_EVENT_RESET:
            state->priv.count = 0;
            return INK_EXAMPLE_OK;
    }
    return INK_EXAMPLE_ERROR_ARGUMENT;
}
