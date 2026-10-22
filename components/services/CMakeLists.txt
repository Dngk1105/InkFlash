set(srcs "ink_service.c")

if(CONFIG_INK_SVC_POWER)
    list(APPEND srcs "svc_power.c")
endif()
if(CONFIG_INK_SVC_GUI)
    list(APPEND srcs "svc_gui.c")
endif()
if(CONFIG_INK_SVC_STORAGE)
    list(APPEND srcs "svc_storage.c")
endif()
if(CONFIG_INK_SVC_AUDIO)
    list(APPEND srcs "svc_audio.c")
endif()
if(CONFIG_INK_SVC_AI)
    list(APPEND srcs "svc_ai.c")
endif()
if(CONFIG_INK_SVC_GESTURE)
    list(APPEND srcs "svc_gesture.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    # Ranh giới firmware/hardware: service chỉ được dùng core, device_registry, ink_hal và ink_gfx (đều không biết phần cứng).
    # KHÔNG thêm bsp, drv_* hay hw_common vào đây.
    REQUIRES core device_registry ink_hal ink_gfx app_update esp_timer
)
