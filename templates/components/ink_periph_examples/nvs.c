/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/storage/nvs/nvs_rw_value. Phải gọi nvs_flash_init() một
 * lần lúc khởi động trước khi dùng. */
#include "ink_periph_examples.h"

#include "nvs.h"

esp_err_t ink_periph_examples_nvs_get_u32(const char *namespace_name,
                                          const char *key,
                                          uint32_t default_value,
                                          uint32_t *out_value) {
  nvs_handle_t handle;
  esp_err_t error = nvs_open(namespace_name, NVS_READONLY, &handle);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    *out_value = default_value; /* namespace chưa có: dùng giá trị mặc định */
    return ESP_OK;
  }
  if (error != ESP_OK) {
    return error;
  }
  error = nvs_get_u32(handle, key, out_value);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    *out_value = default_value;
    error = ESP_OK;
  }
  nvs_close(handle);
  return error;
}

esp_err_t ink_periph_examples_nvs_set_u32(const char *namespace_name,
                                          const char *key, uint32_t value) {
  nvs_handle_t handle;
  esp_err_t error = nvs_open(namespace_name, NVS_READWRITE, &handle);
  if (error != ESP_OK) {
    return error;
  }
  error = nvs_set_u32(handle, key, value);
  if (error == ESP_OK) {
    error = nvs_commit(handle);
  }
  nvs_close(handle);
  return error;
}
