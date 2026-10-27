# Module e-paper WeAct 3.7": làm mới màn

Panel GDEY037T03 (hoặc E037A75, xem `overview.md`), controller UC8253. File này ghi thời gian làm mới đã đo và cách chống bóng mờ.

Cột "Trạng thái": `Đã xác nhận` (đã đo trên máy) hoặc `Chưa xác nhận`.

## Ví dụ

Vẽ một ảnh mới đầy màn bằng sóng chuẩn mất khoảng 2,8 giây, màn chớp đen trắng vài lần. Cập nhật số đếm ở một ô nhỏ bằng partial refresh mất khoảng 0,35 giây và không chớp cả màn; làm nhiều lần thì hình cũ để lại bóng mờ, nên cứ vài chục lần cần một lần làm sạch.

## Thời gian

| Kiểu làm mới | Thời gian | Nguồn | Trạng thái |
|---|---|---|---|
| Full refresh (gồm truyền dữ liệu) | Sóng chuẩn 2,8 s; sóng nhanh 2,3 s. Thư viện ghi ~1,0 s cho riêng phần panel quét | Đo bằng demo, 2026-10-21 | Đã xác nhận |
| Deep clean | 8,3 s (1 vòng đen → trắng, rồi vẽ) | Đo bằng demo | Đã xác nhận |
| Partial refresh | ~0,35 s | GxEPD2 `partial_refresh_time` | Chưa xác nhận |

Tài liệu hãng (panel và datasheet UC8253) không ghi thời gian làm mới. Về nguyên tắc, thời gian bằng số khung hình của bảng sóng dài nhất chia cho tần số khung hình của dải nhiệt độ đang dùng (xem `waveform.md`); hai số này nằm trong OTP của controller và chưa được đọc ra.

## Chống bóng mờ và chấm lốm đốm (kết quả từ bản demo)

- Sóng chuẩn cho full refresh sạch hơn sóng nhanh (bỏ lệnh ép nhiệt độ `E0 02` / `E5 5A`), chậm hơn khoảng 0,5 s.
- Deep clean: đảo cả màn đen rồi trắng trước khi vẽ. Mỗi vòng đen → trắng thêm khoảng 5,5 s. Bản demo xác nhận cách này xoá được chấm lốm đốm ở góc phải.
- Partial refresh nhiều lần sẽ tích bóng mờ; cứ vài chục lần nên deep clean một lần.

## Khuyến nghị của hãng panel

| Khuyến nghị | Nguồn |
|---|---|
| Làm mới ít nhất một lần mỗi 24 giờ; trên 35 °C hiệu ứng điểm đơn không được đảm bảo | Panel tr.11 |
| Độ ẩm 45–70 %RH để giữ chất lượng 5 năm; nên làm mới mỗi ngày | Panel tr.12 |
| Bóng mờ (ΔE) tối đa 2 | Panel tr.12 |

Đối chiếu gốc: `source/panel-e037a75-a0-spec.pdf` tr.11–12; `source/uc8253-datasheet.pdf` tr.22–26, 43–45.
