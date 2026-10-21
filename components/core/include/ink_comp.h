#pragma once

/* Mã component: mỗi khối phần cứng/phần mềm một mã, dùng cho log và health. */
typedef enum {
    INK_COMP_SYS = 0,
    INK_COMP_BUS,
    INK_COMP_CONFIG,
    INK_COMP_BUTTONS,
    INK_COMP_EPD,
    INK_COMP_IMU,
    INK_COMP_CHARGER,
    INK_COMP_AUDIO_HW,
    INK_COMP_SD,
    INK_COMP_LED,
    INK_COMP_SVC_POWER,
    INK_COMP_SVC_GUI,
    INK_COMP_SVC_STORAGE,
    INK_COMP_SVC_AUDIO,
    INK_COMP_SVC_AI,
    INK_COMP_SVC_GESTURE,
    INK_COMP_REMOTE,
    INK_COMP_COUNT,
} ink_comp_t;

const char *ink_comp_name(ink_comp_t comp);
