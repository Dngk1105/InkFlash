idf_component_register(
    SRCS "flashcard/app_flashcard.c" "settings/app_settings.c"
    INCLUDE_DIRS "include"
    # App chỉ thấy app_fw, API service, domain, ui. KHÔNG gọi HAL hay driver.
    REQUIRES app_fw services domain ui core
)
