#pragma once

/*
 * Mã component: mỗi khối phần mềm/phần cứng một mã, dùng chung cho log và health.
 * Thêm khối mới: thêm vào enum và vào bảng tên trong ink_log.c.
 */
typedef enum {
    INK_COMP_SYS = 0,
    INK_COMP_BUS,
    INK_COMP_CONFIG,
    /* phần cứng (qua HAL) */
    INK_COMP_DISPLAY,
    INK_COMP_BUTTON,
    INK_COMP_AUDIO_OUT,
    INK_COMP_AUDIO_IN,
    INK_COMP_IMU,
    INK_COMP_POWER_HW,
    INK_COMP_LED,
    /* service */
    INK_COMP_SVC_INPUT,
    INK_COMP_SVC_RENDER,
    INK_COMP_SVC_STORAGE,
    INK_COMP_SVC_AUDIO,
    INK_COMP_SVC_POWER,
    INK_COMP_SVC_TRANSFER,
    INK_COMP_SVC_SYSTEM,
    /* ứng dụng */
    INK_COMP_APP_MANAGER,
    INK_COMP_APP,
    INK_COMP_COUNT,
} ink_comp_t;

const char *ink_comp_name(ink_comp_t comp);
