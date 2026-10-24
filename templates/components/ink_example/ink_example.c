/* Thiết kế: docs/design/code-structure.md */
#include "ink_example.h"

ink_example_err_t ink_example_init(ink_example_state_t *state, uint32_t limit)
{
    if (state == nullptr || limit == 0) {
        return INK_EXAMPLE_ERR_ARG;
    }
    state->count = 0;
    state->limit = limit;
    return INK_EXAMPLE_OK;
}

ink_example_err_t ink_example_handle(ink_example_state_t *state, const ink_example_event_t *event,
                                     ink_example_action_t *out_action)
{
    if (state == nullptr || event == nullptr || out_action == nullptr) {
        return INK_EXAMPLE_ERR_ARG;
    }
    *out_action = INK_EXAMPLE_ACTION_NONE;
    switch (event->type) {
    case INK_EXAMPLE_EVENT_TICK:
        state->count++;
        if (state->count >= state->limit) {
            state->count = 0;
            *out_action = INK_EXAMPLE_ACTION_NOTIFY;
        }
        return INK_EXAMPLE_OK;
    case INK_EXAMPLE_EVENT_RESET:
        state->count = 0;
        return INK_EXAMPLE_OK;
    }
    return INK_EXAMPLE_ERR_ARG;
}
