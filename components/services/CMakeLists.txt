idf_component_register(
    SRCS "ink_service.c" "svc_input.c" "svc_render.c" "svc_storage.c" "svc_audio.c" "svc_power.c" "svc_transfer.c"
    INCLUDE_DIRS "include"
    # REQUIRES (công khai): chỉ thứ mà header API của service lộ ra (core, ui, domain).
    REQUIRES core ui domain
    # PRIV_REQUIRES: ink_hal để RIÊNG service thấy; app dùng service sẽ KHÔNG thấy HAL.
    # KHÔNG bao giờ thêm bsp hay drv_* vào đây.
    PRIV_REQUIRES ink_hal esp_timer
)
