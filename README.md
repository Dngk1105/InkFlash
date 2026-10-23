# InkFlash

Thiết bị học flashcard bỏ túi dùng màn e-ink, ESP32-S3, pin dùng nhiều tuần, chạy đầy đủ khi offline.

## Bắt đầu

| Việc | Lệnh / tài liệu |
|---|---|
| Kiến trúc firmware | [docs/firmware/architecture.md](docs/firmware/architecture.md) |
| Thông số phần cứng, pinmap | [docs/hardware/](docs/hardware/) |
| Viết một driver | [components/drivers/README.md](components/drivers/README.md) |
| Build (ESP-IDF v6.1) | `idf.py build` |
| Nạp và xem log | `idf.py -p COM4 flash monitor` |
| Host test (domain, ui) | xem mục "Kiểm tra" trong tài liệu kiến trúc |

Console trên máy (`ink>`): `status`, `heap`, `tasks`, `log`, `intent <tên>`, `help`.

## Phiên bản

- `master` + tag `demo-v0`: bản demo phần cứng (benchmark màn e-paper, IMU, loa).
- `arch-v2`: kiến trúc hoàn chỉnh, đang xây theo lộ trình trong tài liệu kiến trúc.
