set(srcs "drv_audio.c")

# Mic và loa chọn độc lập. Thêm chip: một file + một khối if ở đây + Kconfig + extern/define trong drv_audio.c.
if(CONFIG_INK_MIC_FAKE)
    list(APPEND srcs "mic_fake.c")
endif()
if(CONFIG_INK_MIC_INMP441)
    list(APPEND srcs "mic_inmp441.c")
endif()
if(CONFIG_INK_SPEAKER_FAKE)
    list(APPEND srcs "speaker_fake.c")
endif()
if(CONFIG_INK_SPEAKER_MAX98357A)
    list(APPEND srcs "speaker_max98357a.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ink_hal core device_registry hw_common esp_driver_gpio esp_driver_i2s
)
