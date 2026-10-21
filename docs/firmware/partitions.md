# Bảng phân vùng flash 16MB

Bảng phân vùng chia flash thành các vùng có công dụng riêng. File nguồn là `partitions.csv` ở gốc project.

| Tên | Loại | Kích thước | Công dụng |
|---|---|---|---|
| nvs | data/nvs | 16K | Cấu hình, trạng thái nhỏ (key-value) |
| otadata | data/ota | 8K | Ghi firmware nào đang chạy (ota_0 hay ota_1) |
| phy_init | data/phy | 4K | Hiệu chuẩn RF |
| ota_0 | app/ota_0 | 3M | Firmware, bắt đầu ở 0x10000 |
| ota_1 | app/ota_1 | 3M | Firmware dự phòng cho OTA |
| nvs_keys | data/nvs_keys | 4K | Khoá mã hoá NVS (dùng khi bật flash encryption) |
| coredump | data/coredump | 64K | Bản ghi lúc crash, đọc bằng `idf.py coredump-info` |
| model | data/spiffs | 4M | Mô hình ESP-SR (MultiNet7) |
| storage | data/littlefs | ~5.8M | Deck, ReviewLog, media (LittleFS) |

Kiểm tra bảng sau khi sửa: chạy `idf.py partition-table`.
