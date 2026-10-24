/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/storage/nvs/nvs_rw_value. Phải gọi nvs_flash_init() một lần lúc khởi động trước khi dùng. */
#include "ink_periph_examples.h"

#include "nvs.h"

esp_err_t ink_periph_examples_nvs_get_u32(const char *ns, const char *key, uint32_t def, uint32_t *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = def; /* namespace chưa có: dùng giá trị mặc định */
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_u32(h, key, out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = def;
        err = ESP_OK;
    }
    nvs_close(h);
    return err;
}

esp_err_t ink_periph_examples_nvs_set_u32(const char *ns, const char *key, uint32_t value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
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
