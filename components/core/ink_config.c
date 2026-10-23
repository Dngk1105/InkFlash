#include "ink_config.h"
#include "ink_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define NS "ink"

esp_err_t ink_config_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        INK_LOGW(INK_COMP_CONFIG, "xoa NVS: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t ink_config_get_u32(const char *key, uint32_t *out, uint32_t def)
{
    *out = def;
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK; /* namespace chưa có: dùng mặc định */
    }
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_u32(h, key, out);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = def;
        return ESP_OK;
    }
    return err;
}

esp_err_t ink_config_set_u32(const char *key, uint32_t value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_u32(h, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}
