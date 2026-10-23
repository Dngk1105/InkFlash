idf_component_register(
    SRCS "ink_bus.c" "ink_log.c" "ink_health.c" "ink_config.c" "ink_heap.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event nvs_flash esp_timer log heap
)
