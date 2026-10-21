#include "ink_device.h"
#include "ink_health.h"
#include "ink_log.h"

#define MAX_DEVICES 16
#define RETRIES 2

static ink_device_t s_devs[MAX_DEVICES];
static int s_count;

esp_err_t ink_device_register(const ink_device_t *dev)
{
    if (s_count >= MAX_DEVICES || !dev || !dev->ops) {
        return ESP_ERR_NO_MEM;
    }
    s_devs[s_count++] = *dev;
    return ESP_OK;
}

static esp_err_t try_bringup(const ink_device_t *d)
{
    esp_err_t err = d->ops->init ? d->ops->init(d->ctx) : ESP_OK;
    if (err == ESP_OK && d->ops->self_test) {
        err = d->ops->self_test(d->ctx);
    }
    return err;
}

/* Thang phục hồi: thử lại → reset bus → bật/tắt nguồn → DEGRADED. */
static esp_err_t bringup_with_recovery(const ink_device_t *d)
{
    for (int i = 0; i <= RETRIES; i++) {
        if (try_bringup(d) == ESP_OK) {
            return ESP_OK;
        }
    }
    for (int level = 1; level <= 2 && d->ops->recover; level++) {
        INK_LOGW(d->comp, INK_LOG_EVT_RECOVER, "%s: recover level %d", d->name, level);
        if (d->ops->recover(d->ctx, level) == ESP_OK && try_bringup(d) == ESP_OK) {
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t ink_device_init_all(void)
{
    esp_err_t result = ESP_OK;
    for (int i = 0; i < s_count; i++) {
        const ink_device_t *d = &s_devs[i];
        if (bringup_with_recovery(d) == ESP_OK) {
            ink_health_set(d->comp, INK_HEALTH_OK);
            INK_LOGI(d->comp, INK_LOG_EVT_INIT, "%s: OK", d->name);
        } else {
            INK_LOGE(d->comp, INK_LOG_EVT_SELF_TEST_FAIL, "%s: self-test fail", d->name);
            ink_health_set(d->comp, INK_HEALTH_DEGRADED);
            if (!d->optional) {
                result = ESP_FAIL;
            }
        }
    }
    return result;
}

void ink_device_suspend_all(void)
{
    for (int i = s_count - 1; i >= 0; i--) {
        if (s_devs[i].ops->suspend) {
            s_devs[i].ops->suspend(s_devs[i].ctx);
        }
    }
}

void ink_device_resume_all(void)
{
    for (int i = 0; i < s_count; i++) {
        if (s_devs[i].ops->resume) {
            s_devs[i].ops->resume(s_devs[i].ctx);
        }
    }
}

void ink_device_foreach(void (*cb)(const ink_device_t *dev, void *arg), void *arg)
{
    for (int i = 0; i < s_count; i++) {
        cb(&s_devs[i], arg);
    }
}
