# Kiến trúc firmware

## Ví dụ một luồng: bấm nút B để lật thẻ

1. Driver nút trong `bsp` phát hiện cạnh xuống, gửi event `INK_EVT_BUTTON` lên event bus.
2. Service GUI nhận event, hỏi Storage+SRS thẻ tiếp theo (cũng qua event bus), rồi vẽ lên e-paper.
3. Service Power nhận cùng event đó và đặt lại bộ đếm tự ngủ.

Không service nào gọi thẳng hàm của service khác. Nhờ vậy một service lỗi không kéo theo service khác.

## 5 tầng

| Tầng | Thư mục | Vai trò |
|---|---|---|
| Cloud Server | (repo khác) | Đồng bộ, chấm phát âm, TTS, LLM |
| Remote | `components/remote` | WiFi, sync, OTA, hàng đợi offline |
| Services | `components/services` | GUI, Audio, AI, Storage+SRS, Power. Mỗi service một FreeRTOS task |
| Core | `components/core` | Event bus, log, health, config (NVS) |
| HAL/driver + BSP | `components/drivers`, `components/bsp` | Driver linh kiện; BSP (Board Support Package) mô tả chân nối của từng board |

## Ranh giới firmware / hardware

Mục tiêu: đổi linh kiện hay đổi chân thì logic (service) không phải sửa.

```
Service        chỉ dùng core, device_registry, ink_hal            (logic: cử chỉ, ngủ, ôn thẻ)
   ↓ gọi
ink_hal        giao diện chuẩn theo loại linh kiện (đơn vị, hệ trục cố định)
   ↑ thực hiện
drv_*          mỗi chip một file, chọn bằng Kconfig                (biết thanh ghi của chip)
   ↑ cấu hình
bsp            file board: chân, địa chỉ I2C, hướng gắn chip       (biết dây nối)
```

Quy tắc:
1. **Service không được `REQUIRES` bsp hay drv_\***. CMake chặn: include sẽ báo "No such file". Đã thử.
2. **Driver không tự đọc macro chân.** BSP truyền chân qua struct cấu hình (ví dụ `drv_imu_cfg_t`).
3. **HAL quy định đơn vị và hệ trục** (IMU: mg, z hướng ra ngoài màn hình). Driver tự đổi từ hệ trục của chip. Hướng gắn chip khai báo bằng `BSP_IMU_MOUNT` trong file board.
4. **Chân chỉ khai báo ở `components/bsp/include/boards/<board>.h`.** `bsp_pins.h` kiểm tra lúc biên dịch (chân cấm GPIO22–37, nút phải là RTC GPIO, đo pin phải là ADC1 vì ADC2 tranh với WiFi) và lúc khởi động (hai chức năng trùng chân, trừ cặp được phép chung).
5. **Chưa có linh kiện thì dùng driver giả lập** (`INK_IMU_FAKE`). Thử logic bằng console: `imu 0 0 -1000` giả lập úp màn hình, sau 1 giây máy tự gửi yêu cầu ngủ.

### Từng khối phần cứng

| Khối | HAL (logic gọi) | Component driver | Chọn chip | Hiện có |
|---|---|---|---|---|
| Nút bấm | event `INK_EVT_BUTTON` (đã chống dội) | `drv_buttons` | không cần (chỉ GPIO) | Driver thật |
| IMU | `ink_hal_imu.h` (mg, độ, hệ trục thiết bị) | `drv_imu` | `INK_IMU_DRIVER` | BNO055 (thật, chưa chạy trên máy); giả lập |
| Màn | `ink_hal_display.h` (framebuffer 1bpp, hỏi kích thước) | `drv_display` | `INK_DISPLAY_DRIVER` | UC8253 / GDEY037T03 (thật, chưa chạy trên máy); giả lập |
| Mic | `ink_hal_audio.h` (PCM 16 bit mono 16 kHz) | `drv_audio` | `INK_MIC_DRIVER` | INMP441 (thật, chưa chạy trên máy); giả lập |
| Loa | `ink_hal_audio.h` | `drv_audio` | `INK_SPEAKER_DRIVER` | MAX98357A (thật, chưa chạy trên máy); giả lập |
| LED trạng thái | `ink_hal_led.h` | `drv_led` | `INK_LED_DRIVER` | WS2812 qua RMT (thật, chưa chạy trên máy) |
| Nguồn (pin, sạc, ngủ, ship mode, nguồn khối phụ) | `ink_hal_power.h` | `drv_power` | `INK_POWER_DRIVER` | ESP32-S3 deep sleep (RTC GPIO wake); giả lập |
| microSD | chưa có | chưa có | | Chưa làm |

Hai component dùng chung, chỉ driver được gọi (service không được):
- `hw_common`: bus I2C dùng chung (`hw_i2c`) và cặp kênh I2S song công cho mic + loa (`hw_i2s`).
- `ink_gfx`: vẽ chữ và hình lên framebuffer 1bpp của HAL màn. Không biết phần cứng nên service được dùng.

Driver giả lập cho logic chạy thử khi chưa có linh kiện: `hw` trong console in mọi thứ nhìn qua HAL.
Mặc định cho breadboard nằm trong `sdkconfig.defaults`; đổi lựa chọn bằng menuconfig (nhớ xoá `sdkconfig` nếu đổi mặc định).
Demo benchmark dùng toàn bộ các khối trên: xem [demo.md](demo.md).

### Việc cần làm khi

| Thay đổi | Sửa ở đâu | Service có phải sửa? |
|---|---|---|
| Đổi chân | `boards/<board>.h` | Không |
| Nút nối kéo lên hay kéo xuống | `BSP_BTN_ACTIVE_LOW` | Không |
| Gắn chip IMU xoay hướng khác | `BSP_IMU_MOUNT` | Không |
| Đổi chip (IMU, màn, mic, loa, chip sạc) | thêm 1 file driver + 3 dòng (khuôn: `components/drivers/drv_imu/README.md`) | Không |
| Mic và loa dùng chung BCLK/WS, hoặc nhiều khối chung một công tắc tải | khai báo cùng chân; cặp hợp lệ nằm trong `BSP_PIN_SHARE_LIST` | Không |
| Thêm loại linh kiện mới (ví dụ cảm biến ánh sáng) | thêm `ink_hal_*.h` + `drv_*` + cấu hình trong `bsp.c` | Chỉ service dùng nó |

Chân board không khai báo thì mặc định `GPIO_NUM_NC`; lúc khởi động BSP ghi một dòng cảnh báo liệt kê chân chưa nối.

## Device registry

Mỗi thiết bị phần cứng đăng ký một bảng hàm `init / self_test / suspend / resume / recover` vào `components/device_registry`. Service Power gọi `suspend` trước khi ngủ và `resume` khi thức.

## Chịu lỗi

Khi một thiết bị lỗi, hệ thống thử lần lượt:
1. Thử lại thao tác.
2. Reset bus (I2C/SPI).
3. Tắt rồi bật nguồn khối đó qua TPS22916.
4. Đánh dấu `DEGRADED`, máy vẫn chạy với các khối còn lại.

Thêm vào đó: watchdog, core dump vào flash, safe mode khi crash liên tục, OTA rollback.

## Log

- Mỗi linh kiện có một mã component, mỗi sự kiện có event ID.
- Log ra console, RAM ring buffer và flash.
- Console chẩn đoán dùng `esp_console`, có lệnh `status`.
