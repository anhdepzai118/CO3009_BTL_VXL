# Hệ Thống Đèn Giao Thông 2 Tuyến Đường Trên Mạch XNUCLEO-F103RB (BTL CO3009)

Bài tập lớn môn **Vi Xử Lý – Vi Điều Khiển (CO3009)** – Trường Đại học Bách Khoa – ĐHQG TP.HCM.

## 1. Giới thiệu dự án
Hệ thống điều khiển đèn giao thông cho ngã tư 2 tuyến đường trên bo mạch **XNUCLEO-F103RB (STM32F103RBT6)** kết hợp mạch mở rộng **Grove Shield**, vận hành hoàn toàn trên bộ lập lịch hợp tác **Scheduler $O(1)$** (không dùng `HAL_Delay`):
- **Chế độ Tự động (`AUTOMATIC`)**: Luân phiên 4 pha `Đỏ 1 – Xanh 2` $\rightarrow$ `Đỏ 1 – Vàng 2` $\rightarrow$ `Xanh 1 – Đỏ 2` $\rightarrow$ `Vàng 1 – Đỏ 2` (mặc định: Đỏ = 5s, Xanh = 3s, Vàng = 2s), hiển thị thời gian đếm ngược của từng tuyến trên LCD 16x2.
- **Chế độ Bằng tay (`MANUAL`)**: Chuyển luồng ưu tiên bằng nút nhấn có chèn pha đệm đèn Vàng an toàn 2 giây; hỗ trợ chế độ cảnh báo nhấp nháy Vàng cả 2 tuyến.
- **Chế độ Chỉnh thời gian (`TUNING`)**: Chỉnh thời gian Đỏ – Vàng – Xanh (1..99s), chớp tắt 2 Hz màu đèn đang chọn, kiểm tra ràng buộc hợp lệ $T_{\text{Đỏ}} = T_{\text{Xanh}} + T_{\text{Vàng}}$.
- **Cảm biến môi trường**: Đọc cảm biến **DHT20** định kỳ (Non-blocking I2C) và hiển thị nhiệt độ ($^\circ\text{C}$), độ ẩm ($\text{\%RH}$) trên màn hình **LCD 16x2 I2C**.

---

## 2. Cấu trúc thư mục
```text
CO3009_BTL_VXL/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── global.h
│   │   ├── scheduler.h
│   │   ├── software_timer.h
│   │   ├── button.h
│   │   ├── traffic_light.h
│   │   ├── dht20.h
│   │   ├── lcd.h
│   │   ├── fsm_automatic.h
│   │   ├── fsm_manual.h
│   │   └── fsm_tuning.h
│   └── Src/
│       ├── main.c
│       ├── global.c
│       ├── scheduler.c
│       ├── software_timer.c
│       ├── button.c
│       ├── traffic_light.c
│       ├── dht20.c
│       ├── lcd.c
│       ├── fsm_automatic.c
│       ├── fsm_manual.c
│       └── fsm_tuning.c
├── Drivers/
├── BTL_VXL.ioc
├── .gitignore
└── README.md
```

---

## 3. Kết nối phần cứng (XNUCLEO-F103RB + Grove Shield)

| Thiết bị | Khe Grove / Arduino | Chân STM32 | Nhãn trong code |
| :--- | :---: | :---: | :--- |
| **Đèn giao thông 1** | `D3 – D4` | `PB3`, `PB5` | `TL1_A`, `TL1_B` |
| **Đèn giao thông 2** | `D5 – D6` | `PB4`, `PB10` | `TL2_A`, `TL2_B` |
| **Module nút nhấn 1** | `D7 – D8` | `PA8`, `PA9` | `BTN_1` (MODE), `BTN_2` (SELECT) |
| **Module nút nhấn 2** | `D9 – D10` | `PC7`, `PB6` | `BTN_3` (INC/+), `BTN_4` (DEC/-) |
| **Nút USER & LED mạch** | Tích hợp | `PC13`, `PA5`, `PC9` | `B1`, `LD2`, `LED_PC9` |
| **LCD 16x2 & DHT20** | Khe `I2C` (Đỏ) | `PB8` (SCL), `PB9` (SDA) | Địa chỉ: `DHT20 = 0x38`, `LCD = 0x21` |

---

## 4. Hướng dẫn Build và Nạp chương trình

1. Clone repository về máy:
   ```bash
   git clone https://github.com/anhdepzai118/CO3009_BTL_VXL.git
   ```
2. Mở **STM32CubeIDE** $\rightarrow$ chọn **File $\rightarrow$ Open Projects from File System...** $\rightarrow$ chọn thư mục `CO3009_BTL_VXL`.
3. Kiểm tra cấu hình `.ioc`:
   - Board: **NUCLEO-F103RB** (`STM32F103RBT6`)
   - `System Core -> SYS -> Debug`: **Serial Wire**
4. Nhấn **`Ctrl + B`** để Build dự án.
5. Kết nối mạch **XNUCLEO-F103RB** qua cổng USB ST-LINK và nhấn **Run (`F11`)** (hoặc nạp file `Debug/BTL_VXL.elf` qua **STM32CubeProgrammer** với tùy chọn *Run after programming*).

---

## 5. Hướng dẫn vận hành

- **Nút 1 (`D7 - PA8`)**: Chuyển chế độ vòng tròn `Tự động (AUTO)` $\rightarrow$ `Bằng tay (MANUAL)` $\rightarrow$ `Chỉnh giờ (TUNING)` $\rightarrow$ `Tự động (AUTO)`.
- **Nút 2 (`D8 - PA9`)**:
  - Trong `MANUAL`: Đổi tuyến đường ưu tiên (có chèn pha đèn Vàng an toàn 2 giây).
  - Trong `TUNING`: Chọn màu đèn cần chỉnh (`Đỏ` $\rightarrow$ `Vàng` $\rightarrow$ `Xanh`).
- **Nút 3 (`D9 - PC7`)**:
  - Trong `TUNING`: Tăng thời gian `+1 giây` (nhấn giữ tự động tăng nhanh).
  - Trong `MANUAL`: Bật/tắt chế độ cảnh báo nhấp nháy đèn Vàng cả 2 tuyến.
- **Nút 4 (`D10 - PB6`)**:
  - Trong `TUNING`: Giảm thời gian `-1 giây` (nhấn giữ tự động giảm nhanh).
- **Nút `USER` (`PC13`)**:
  - Trong `TUNING`: Tự động cân bằng $T_{\text{Đỏ}} = T_{\text{Xanh}} + T_{\text{Vàng}}$, lưu và về `AUTO`.
  - Trong `AUTO`: Khôi phục nhanh bộ thời gian mặc định (`Đỏ 5s, Xanh 3s, Vàng 2s`).
