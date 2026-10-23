#include "ink_log.h"
#include "ink_service.h"
#include "svc_internal.h"
#include "svc_storage.h"

/* GĐ2: mount LittleFS (component joltwire/littlefs), định dạng cards.bin, ghi ReviewLog có fsync. */

bool storage_ready(void)
{
    return false;
}

esp_err_t storage_append_review(const ink_review_t *review)
{
    (void)review;
    return ESP_ERR_NOT_SUPPORTED;
}

static void on_start(void)
{
    INK_LOGW(INK_COMP_SVC_STORAGE, "chua co LittleFS (GD2)");
}

static const ink_service_def_t s_def = {
    .name = "svc_storage",
    .comp = INK_COMP_SVC_STORAGE,
    .stack_size = 4096,
    .priority = 4,
    .on_start = on_start,
};

esp_err_t svc_storage_start(void)
{
    return ink_service_start(&s_def, NULL);
}
