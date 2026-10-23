#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "ink_review_log.h"

/*
 * Storage service: LittleFS trên phân vùng `storage`; deck, ReviewLog (chỉ ghi thêm), media theo hash, trạng thái phiên học.
 * CHƯA VIẾT — giai đoạn 2. API dưới đây chốt hình dạng để app lập trình trước.
 * Sẵn sàng thì phát INK_EVT_STORAGE_READY.
 */
bool storage_ready(void);
esp_err_t storage_append_review(const ink_review_t *review);
