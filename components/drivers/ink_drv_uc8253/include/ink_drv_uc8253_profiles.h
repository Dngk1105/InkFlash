/* Thiết kế: docs/design/display/display.md */
#pragma once

/* Sinh bằng tools/profile_gen.py từ profiles/profiles.json; không sửa tay. */
#include "ink_drv_uc8253.h"

#define INK_DRV_UC8253_PROFILE_COUNT 24

extern const ink_drv_uc8253_profile_t          ink_drv_uc8253_profiles[INK_DRV_UC8253_PROFILE_COUNT];
extern const ink_hal_display_purpose_default_t ink_drv_uc8253_purpose_defaults[INK_HAL_DISPLAY_PURPOSE_COUNT];
