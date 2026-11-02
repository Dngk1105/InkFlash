# Thiết kế: docs/design/display/bench.md
idf_component_register(SRCS "bench_main.c" "bench_link.c" "bench_task.c" "bench_draw.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_app_format esp_driver_usb_serial_jtag esp_timer freertos heap log
                                     ink_bsp ink_display ink_display_policy ink_input ink_platform)
