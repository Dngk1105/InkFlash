# Module e-paper WeAct 3.7": làm mới màn

Panel GDEY037T03, controller UC8253.

Cột "Trạng thái": `Đã xác nhận` (đã đo trên máy) hoặc `Chưa xác nhận`.

## Thời gian

| Kiểu làm mới | Thời gian | Nguồn | Trạng thái |
|---|---|---|---|
| Full refresh (gồm truyền dữ liệu) | Sóng chuẩn 2,8 s; sóng nhanh 2,3 s. Thư viện ghi ~1,0 s cho riêng phần panel quét | Đo bằng demo, 2026-10-21 | Đã xác nhận |
| Deep clean | 8,3 s (1 vòng đen → trắng, rồi vẽ) | Đo bằng demo | Đã xác nhận |
| Partial refresh | ~0,35 s | GxEPD2 `partial_refresh_time` | Chưa xác nhận |

## Chống bóng mờ và chấm lốm đốm (kết quả từ bản demo)

- Sóng chuẩn cho full refresh sạch hơn sóng nhanh (bỏ lệnh ép nhiệt độ `E0 02` / `E5 5A`), chậm hơn khoảng 0,5 s.
- Deep clean: đảo cả màn đen rồi trắng trước khi vẽ. Mỗi vòng đen → trắng thêm khoảng 5,5 s. Bản demo xác nhận cách
  này xoá được chấm lốm đốm ở góc phải.
- Partial refresh nhiều lần sẽ tích bóng mờ; cứ vài chục lần nên deep clean một lần.
