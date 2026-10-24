# Datasheet Processor (Fast DeepSheet Pipeline)

Tool tự động hóa bóc tách và chuẩn hóa tài liệu kỹ thuật PDF (datasheets, reference manuals) của linh kiện điện tử thành định dạng **Markdown GitHub-Flavored (GFM)** sạch sẽ, trực quan cho embedded firmware và hardware engineers.

Sử dụng engine siêu nhẹ **pdfplumber + DeepSheet Synthesizer** (chạy tức thì trong vài giây, 100% offline, **hoàn toàn không cần PyTorch / GPU**).

---

## 🚀 Tính năng nổi bật

1. **Siêu nhẹ & Tốc độ cao**:
   - Khởi động tức thì, chạy 100% offline không cần tải mô hình AI nặng hàng gigabyte.
   - Quét 8 file datasheet (hơn 300 trang) chỉ mất **~45 giây**.

2. **Lọc trang thông minh (Smart Page Slicing)**:
   - Tự động nhận diện và chỉ trích xuất các trang kỹ thuật cốt lõi: Sơ đồ chân (*Pinout*), Giới hạn cực đại (*Absolute Maximum Ratings*), Điều kiện hoạt động (*Operating Conditions*), Đặc tính điện (*Electrical Characteristics*), Bản đồ thanh ghi (*Register Maps*).
   - Tự động bỏ qua các trang bìa, pháp lý, đóng gói.

3. **Chuẩn hóa & Tổng hợp thông số (DeepSheet Synthesizer)**:
   - Lọc sạch toàn bộ rác OCR, chữ tiếng Trung lặp lại đầu trang và số liệu bản vẽ FPC.
   - Tự động đóng gói thành bảng thông số kỹ thuật 1 trang dễ đọc.
   - Chuẩn hóa ký tự kỹ thuật: `uA` → `µA`, `ohm` → `Ω`, `degC` → `°C`, `+/-` → `±`.

---

## 📁 Cấu trúc thư mục

```
datasheet_processor/
├── pyproject.toml              # Quản lý dependencies siêu nhẹ
├── README.md                   # Hướng dẫn sử dụng chi tiết
├── main.py                     # Điểm khởi chạy CLI
├── input_datasheets/           # Thư mục chứa các file PDF đầu vào cần quét
├── output_markdown/            # Thư mục xuất file .md kết quả
└── src/
    ├── config.py               # Từ điển ký hiệu kỹ thuật & regex nhận diện
    ├── page_router.py          # Quét lướt và lọc trang kỹ thuật
    ├── docling_engine.py       # Engine bóc tách bảng & layout
    ├── deepsheet_normalizer.py # Bộ chuẩn hóa đơn vị và cấu trúc tài liệu
    ├── synthesizer.py          # Bộ tổng hợp Markdown tóm tắt 1 trang
    ├── pipeline.py             # Pipeline tổng hợp end-to-end
    └── cli.py                  # Giao diện dòng lệnh Typer
```

---

## 🛠 Cài đặt & Sử dụng

### 1. Cài đặt môi trường (siêu nhanh, chỉ vài giây)
```bash
cd E:\ESP32_projects\InkFlash\tools\datasheet_processor
uv venv
uv pip install -e .
```

### 2. Quét thử nhanh mục lục (Inspect)
```bash
.venv\Scripts\python main.py inspect input_datasheets/BQ25180.pdf
```

### 3. Trích xuất một file PDF đơn lẻ
```bash
.venv\Scripts\python main.py extract input_datasheets/BQ25180.pdf -o output_markdown/
```

### 4. Quét hàng loạt tất cả file PDF trong thư mục
```bash
.venv\Scripts\python main.py extract "E:\ESP32\Tài liệu cho ProjectIII\WeAct E-Paper Module 3.7 inch\Doc" -o output_markdown/weact_3_7/
```
