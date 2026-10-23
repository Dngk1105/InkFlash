#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "ink_comp.h"

/*
 * Runtime chung cho service: mỗi service = một FreeRTOS task + một queue, sống suốt từ lúc boot.
 * Nguồn message vào queue:
 *  - Sự kiện bus mà service đăng ký (`events`): handler của bus chỉ chép vào queue, không xử lý.
 *  - Lệnh từ hàm API của chính service đó (ink_service_send).
 *  - Lệnh vòng đời của hệ thống: SUSPEND/RESUME khi chuyển trạng thái nguồn.
 * Xem docs/adr/0004-vong-doi-app-task.md.
 */
#define INK_SVC_DATA_MAX 24

typedef enum {
    INK_SVC_MSG_EVENT,   /* sự kiện từ bus; `id` = ink_evt_id_t */
    INK_SVC_MSG_CMD,     /* lệnh riêng của service; `id` do service tự định nghĩa */
    INK_SVC_MSG_SUSPEND,
    INK_SVC_MSG_RESUME,
} ink_svc_msg_type_t;

typedef struct {
    uint8_t type; /* ink_svc_msg_type_t */
    int32_t id;
    uint8_t data[INK_SVC_DATA_MAX];
} ink_svc_msg_t;

typedef struct ink_service ink_service_t;

typedef struct {
    const char *name;
    ink_comp_t comp;
    uint32_t stack_size;
    uint8_t priority;
    uint8_t queue_len;
    const int32_t *events; /* danh sách sự kiện bus cần nghe, kết thúc bằng -1; NULL = không nghe */
    uint32_t tick_ms;      /* 0 = không tick */
    void (*on_start)(void);
    void (*on_msg)(const ink_svc_msg_t *msg);
    void (*on_tick)(void);
} ink_service_def_t;

esp_err_t ink_service_start(const ink_service_def_t *def, ink_service_t **out);
esp_err_t ink_service_send(ink_service_t *svc, ink_svc_msg_type_t type, int32_t id, const void *data, size_t size);

/* Khởi động toàn bộ service theo thứ tự phụ thuộc. */
esp_err_t ink_services_start(void);
/* Gửi SUSPEND/RESUME tới mọi service (service power gọi trước khi ngủ / sau khi thức). */
void ink_services_broadcast(ink_svc_msg_type_t type);
