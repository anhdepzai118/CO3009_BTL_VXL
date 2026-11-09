#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#include <stdint.h>

/* ==========================================================================
 * CÁC TRẠNG THÁI CỦA HỆ THỐNG MÁY TRẠNG THÁI (FSM STATES)
 * ========================================================================== */
#define INIT                0

/* 1. Chế độ Tự động (Automatic Mode) */
#define AUTO_RED1_GREEN2    1   // Tuyến 1 Đỏ   - Tuyến 2 Xanh
#define AUTO_RED1_AMBER2    2   // Tuyến 1 Đỏ   - Tuyến 2 Vàng
#define AUTO_GREEN1_RED2    3   // Tuyến 1 Xanh - Tuyến 2 Đỏ
#define AUTO_AMBER1_RED2    4   // Tuyến 1 Vàng - Tuyến 2 Đỏ

/* 2. Chế độ Bằng tay (Manual Mode) */
#define MAN_RED1_GREEN2     11  // Giữ Tuyến 1 Đỏ - Tuyến 2 Xanh
#define MAN_RED1_AMBER2     12  // Pha đệm an toàn 2s: Tuyến 1 Đỏ - Tuyến 2 Vàng
#define MAN_GREEN1_RED2     13  // Giữ Tuyến 1 Xanh - Tuyến 2 Đỏ
#define MAN_AMBER1_RED2     14  // Pha đệm an toàn 2s: Tuyến 1 Vàng - Tuyến 2 Đỏ
#define MAN_BLINK_AMBER     15  // Chế độ cảnh báo: Nhấp nháy Vàng cả 2 tuyến

/* 3. Chế độ Chỉnh thời gian (Tuning Mode) */
#define TUNE_RED            21  // Chỉnh thời gian đèn Đỏ
#define TUNE_AMBER          22  // Chỉnh thời gian đèn Vàng
#define TUNE_GREEN          23  // Chỉnh thời gian đèn Xanh

/* ==========================================================================
 * CÁC BIẾN TOÀN CỤC DÙNG CHUNG GIỮA CÁC FSM
 * ========================================================================== */
extern int status;              // Trạng thái hiện tại của hệ thống

/* Thời gian cài đặt chính thức (đơn vị: giây) - Luôn đảm bảo time_red == time_green + time_amber */
extern int time_red;
extern int time_amber;
extern int time_green;

/* Thời gian tạm trong chế độ Tuning (đơn vị: giây) */
extern int temp_red;
extern int temp_amber;
extern int temp_green;

/* Biến đếm ngược thời gian (đơn vị: giây) cho Tuyến 1 và Tuyến 2 */
extern int countdown1;
extern int countdown2;

/* Biến đếm thời gian duy trì pha trong chế độ Manual (đơn vị: giây) */
extern int manual_elapsed;

/* Cờ thông báo tạm thời trên LCD (được tự động xóa bởi One-shot Task trên Scheduler) */
extern uint8_t lcd_notice_code; // 0: Bình thường, 1: SAVE OK, 2: ERR R!=G+Y, 3: RESET DEFAULT

/* Hàm tiện ích khởi tạo lại chu kỳ Tự động từ đầu */
void global_enter_auto_mode(void);

/* Hàm callback cho One-shot Task để xóa thông báo tạm trên LCD */
void global_clear_lcd_notice_oneshot(void);

#endif /* INC_GLOBAL_H_ */
