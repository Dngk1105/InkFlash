set(srcs "ink_service.c"
         "svc_power.c" "svc_gui.c" "svc_storage.c" "svc_audio.c" "svc_ai.c" "svc_gesture.c")
if(CONFIG_INK_DEMO)
    list(APPEND srcs "svc_demo.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    # Ranh giới firmware/hardware: service chỉ được dùng core, device_registry, ink_hal và ink_gfx (đều không biết phần cứng).
    # KHÔNG thêm bsp, drv_* hay hw_common vào đây.
    REQUIRES core device_registry ink_hal ink_gfx app_update esp_timer
)
