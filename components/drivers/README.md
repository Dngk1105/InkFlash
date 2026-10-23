# Drivers (người dùng viết)

Thư mục này chứa driver linh kiện. **Hiện để trống có chủ đích**: tầng dưới được viết lại từ đầu và tối ưu riêng.
Code driver của bản demo nằm ở tag `demo-v0` (ví dụ `git show demo-v0:components/drivers/drv_display/display_uc8253.c`)
để tham khảo.

CMake gốc đã khai báo `EXTRA_COMPONENT_DIRS "components/drivers"`, nên mỗi thư mục con có `CMakeLists.txt` là một component.

## Hợp đồng một driver phải theo

1. **Một component cho một chip**, tên `drv_<chip>` (ví dụ `drv_uc8253`, `drv_bno055`). Không trùng tên component
   của ESP-IDF (đã gặp: `hal`).
2. **Thực hiện interface HAL** trong `components/ink_hal/include/ink_hal_*.h`: điền struct `..._ops` và gọi
   `ink_<loại>_attach(ops, ctx)` trong hàm `init`.
3. **Vòng đời**: cung cấp `ink_device_ops_t` (`init / self_test / suspend / resume / recover`) và đăng ký bằng
   `ink_device_register()`. Hàm nào không cần thì để NULL.
4. **Nhận chân qua struct cấu hình** (`drv_<chip>_cfg_t`) do BSP điền. Driver **không** include `bsp_pins.h`.
5. **Hàm public duy nhất** là `esp_err_t drv_<chip>_register(const drv_<chip>_cfg_t *cfg);`, khai báo trong
   `include/drv_<chip>.h`. Mọi thứ khác là `static`.
6. **Bộ nhớ** (docs/adr/0003-cap-phat-dong.md): buffer DMA cấp một lần trong `init` bằng
   `heap_caps_malloc(MALLOC_CAP_DMA)`; không cấp phát trong ISR hay vòng lặp truyền dữ liệu.
7. **Bus dùng chung** (I2C cho IMU + chip sạc, I2S cho mic + loa): không tự tạo bus nếu khối khác cũng dùng.
   Tạo component `hw_bus` khi gặp trường hợp đầu tiên.

## Driver cần có cho MVP

| Driver | HAL | Giai đoạn | Tham khảo |
|---|---|---|---|
| `drv_uc8253` (màn GDEY037T03) | `ink_hal_display.h` | GĐ1 | `docs/hardware/display-weact-3.7.md`, demo-v0 |
| `drv_buttons_gpio` | `ink_hal_button.h` | GĐ1 | demo-v0 `drv_buttons` |
| `drv_max98357a` (loa) | `ink_hal_audio.h` | GĐ5 | demo-v0 `drv_audio` |
| `drv_esp32_power` (deep sleep, đọc nguyên nhân thức) | `ink_hal_power.h` | GĐ6 | |
| `drv_ws2812` (LED, tuỳ chọn) | `ink_hal_led.h` | bất kỳ | demo-v0 `drv_led` |
