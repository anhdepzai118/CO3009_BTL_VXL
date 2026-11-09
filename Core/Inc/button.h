#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>

#define NUM_BUTTONS     5

/* Định danh các nút nhấn trong hệ thống */
#define BTN_MODE        0   // Nút 1 (D7 - PA8): Chuyển chế độ (Auto -> Manual -> Tuning -> Auto)
#define BTN_SELECT      1   // Nút 2 (D8 - PA9): Đổi làn (Manual) / Chọn đèn chỉnh (Tuning)
#define BTN_INC         2   // Nút 3 (D9 - PC7): Tăng thời gian (+1s) / Bật nháy Vàng (Manual)
#define BTN_DEC         3   // Nút 4 (D10 - PB6): Giảm thời gian (-1s)
#define BTN_USER_B1     4   // Nút USER (PC13): Lưu nhanh / Khôi phục mặc định

/* Khởi tạo module nút nhấn và tự động lấy mẫu mức logic nhả phím */
void button_init(void);

/* Đọc trạng thái các nút nhấn mỗi 10ms, chống dội 3 mẫu liên tiếp và phát hiện nhấn giữ */
void button_reading(void);

/* Trả về 1 nếu nút index vừa được nhấn (tự động xóa cờ sau khi đọc) */
int isButtonPressed(int index);

/* Trả về 1 nếu nút index đang được nhấn giữ lâu (> 1 giây, lặp lại mỗi 250ms) */
int isButtonLongPressed(int index);

/* Xóa toàn bộ cờ nút nhấn (gọi khi chuyển chế độ lớn để tránh sót sự kiện) */
void button_clear_all(void);

#endif /* INC_BUTTON_H_ */
