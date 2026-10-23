idf_component_register(
    SRCS "inkflash_main.c" "console.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES core ink_hal bsp services app_fw apps console app_update esp_system
)
