#include "ink_hal_display.h"
#include "ink_log.h"
#include "ink_service.h"

/*
 * GUI: vẽ thẻ lên màn qua HAL. Không giả định độ phân giải: hỏi ink_display_get_info().
 * TODO: render thẻ vào framebuffer 1bpp rồi ink_display_draw().
 */
static void on_start(void)
{
    ink_display_info_t info;
    if (ink_display_get_info(&info) == ESP_OK) {
        INK_LOGI(INK_COMP_SVC_GUI, INK_LOG_EVT_STATE, "display %ux%u partial=%d", info.width, info.height,
                 info.partial_refresh);
    } else {
        INK_LOGW(INK_COMP_SVC_GUI, INK_LOG_EVT_STATE, "no display");
    }
}

static void on_event(const ink_svc_msg_t *msg)
{
    if (msg->id == INK_EVT_BUTTON) {
        const ink_button_evt_t *b = (const ink_button_evt_t *)msg->data;
        if (b->action == INK_BTN_PRESS) {
            INK_LOGI(INK_COMP_SVC_GUI, INK_LOG_EVT_STATE, "button %c", b->button == INK_BTN_A ? 'A' : 'B');
        }
    }
}

const ink_service_def_t svc_gui_def = {
    .name = "svc_gui",
    .comp = INK_COMP_SVC_GUI,
    .stack_size = 4096,
    .priority = 4,
    .on_start = on_start,
    .on_event = on_event,
};
