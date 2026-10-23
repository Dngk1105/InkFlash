idf_component_register(
    SRCS "app_manager.c" "ink_scene.c"
    INCLUDE_DIRS "include"
    REQUIRES core services ui esp_system heap
)
