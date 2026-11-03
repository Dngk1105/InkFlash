/* Thiết kế: docs/design/display/bench.md */

/* app_main chỉ lắp các khối với nhau: bsp, rồi task bench theo đường vẽ đã chọn. */
#include "bench.h"

#include "esp_err.h"
#include "ink_bsp.h"

void app_main(void) {
    ESP_ERROR_CHECK(ink_bsp_init());
    ESP_ERROR_CHECK(bench_task_start(bench_saved_path()));
}
