# Hệ thống Kiểm soát Cửa Thông minh ESP32 + Mã PIN (Wokwi)

**Đồ án môn học: Lập trình IoT**  
**Sinh viên:** Lê Phước Hào  
**MSSV:** DPM235507  
**Lớp:** DH24PM  
**Giảng viên hướng dẫn:** Huỳnh Lý Thanh Nhàn  
**Trường:** Đại học An Giang – Khoa Công nghệ Thông tin  

---

## 1. Mô tả đề tài

Đồ án thiết kế và lập trình một **hệ thống kiểm soát cửa thông minh** sử dụng vi điều khiển **ESP32**, bàn phím ma trận 4x4 và mã PIN. Hệ thống được mô phỏng hoàn toàn trên nền tảng **Wokwi**.

### Chức năng chính

| Chức năng | Mô tả |
|-----------|-------|
| Nhập PIN | Nhận mã 4 số từ keypad 4×4 |
| Xác thực | So sánh với PIN cấu hình sẵn (`1234`) |
| Mở cửa | Servo quay 90° khi nhập đúng PIN |
| Tự đóng cửa | Sau 5 giây servo quay về 0° |
| Cảnh báo sai PIN | LED đỏ + buzzer, đếm số lần sai |
| Khóa tạm thời | Sau 3 lần sai liên tiếp → khóa 30 giây |
| Hiển thị trạng thái | LED xanh/đỏ + Serial Monitor (không in PIN) |

> **Lưu ý:** Đây là sản phẩm phục vụ mục đích học tập. Mã PIN cố định trong code, chỉ chạy trên mô phỏng Wokwi, **không phải hệ thống an ninh thực tế**.

---

## 2. Linh kiện sử dụng (mô phỏng)

- ESP32 DevKit V1
- Membrane Keypad 4×4
- Servo SG90
- LED xanh + LED đỏ (qua điện trở 220Ω)
- Buzzer

---

## 3. Cấu trúc thư mục

```
├── README.md
└── Chuong_Trinh_Wokwi/
    ├── sketch.ino          # Mã nguồn Arduino (ESP32)
    ├── diagram.json        # Sơ đồ mạch Wokwi
    ├── libraries.txt       # Thư viện cần thiết
    └── wokwi-project.txt   # Link dự án Wokwi
```

---

## 4. Cách chạy mô phỏng trên Wokwi

1. Truy cập dự án gốc:  
   👉 [https://wokwi.com/projects/478311706878221313](https://wokwi.com/projects/478311706878221313)

2. Hoặc tạo project mới trên Wokwi và copy nội dung các file trong thư mục `Chuong_Trinh_Wokwi/`:
   - `sketch.ino`
   - `diagram.json`
   - `libraries.txt`

3. Nhấn **Start Simulation**.

### Hướng dẫn sử dụng

| Phím | Chức năng |
|------|-----------|
| `0-9` | Nhập số PIN |
| `A`   | Xác nhận PIN |
| `*`   | Xóa bộ đệm nhập |
| `#`   | (dự phòng) |

- PIN mặc định: **`1234`**
- Nhập đúng → cửa mở 5 giây rồi tự đóng
- Sai 3 lần liên tiếp → hệ thống khóa 30 giây

---

## 5. Sơ đồ trạng thái

```
LOCKED ──(nhập số)──► INPUTTING ──(A + đúng PIN)──► UNLOCKED
   ▲                      │                              │
   │                      │ (A + sai PIN)                │ (sau 5s)
   │                      ▼                              │
   └────────── LOCKOUT ◄──┘                              │
        (sau 3 lần sai)                                  │
              │                                          │
              └──────────(hết 30s)───────────────────────┘
```

---

## 6. Thư viện sử dụng

- `Keypad` (bàn phím ma trận)
- `ESP32Servo`

---

## 7. Tài liệu đầy đủ

- **Báo cáo Word + Video demo** được lưu tại Google Drive:  
  👉 [https://drive.google.com/drive/folders/1uLDTA4ZRdfVtbqNTAgZkxRoCq9w2J6Tz](https://drive.google.com/drive/folders/1uLDTA4ZRdfVtbqNTAgZkxRoCq9w2J6Tz)

- **Link Wokwi gốc:** https://wokwi.com/projects/478311706878221313

---

## 8. Tác giả

**Lê Phước Hào**  
MSSV: DPM235507  
Lớp: DH24PM – Đại học An Giang  

© 2026 – Đồ án phục vụ mục đích học tập
