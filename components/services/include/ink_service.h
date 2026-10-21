#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "ink_bus.h"
#include "ink_comp.h"

#define INK_SVC_MSG_DATA_MAX 16

/* Một event từ bus, đã chép vào hàng đợi riêng của service. */
typedef struct {
    int32_t id;
    uint8_t data[INK_SVC_MSG_DATA_MAX];
} ink_svc_msg_t;

/*
 * Mỗi service = một FreeRTOS task + một hàng đợi.
 * Handler của bus chỉ chép event vào hàng đợi, nên bus không bị service chậm chặn lại.
 */
typedef struct {
    const char *name;
    ink_comp_t comp;
    uint32_t stack_size;
    int priority;
    void (*on_start)(void);
    void (*on_event)(const ink_svc_msg_t *msg);
    uint32_t tick_ms;      /* 0 = không có tick */
    void (*on_tick)(void); /* gọi mỗi tick_ms khi không có event */
} ink_service_def_t;

esp_err_t ink_service_start(const ink_service_def_t *def);

/* Khởi động toàn bộ service. */
esp_err_t ink_services_start(void);

/* Định nghĩa của từng service (svc_*.c). */
extern const ink_service_def_t svc_power_def;
extern const ink_service_def_t svc_gui_def;
extern const ink_service_def_t svc_storage_def;
extern const ink_service_def_t svc_audio_def;
extern const ink_service_def_t svc_ai_def;
extern const ink_service_def_t svc_gesture_def;
extern const ink_service_def_t svc_demo_def; /* chỉ có khi CONFIG_INK_DEMO */
