#include "fsm_tuning.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "scheduler.h"

void fsm_tuning_enter(void)
{
    temp_red   = time_red;
    temp_amber = time_amber;
    temp_green = time_green;

    /* Bỏ qua bước chỉnh Đỏ, vào thẳng chế độ chỉnh Xanh */
    status = TUNE_GREEN;
    clearTimer(0);
    clearTimer(1);
    setTimer(2, 250); // Chớp tắt 2 Hz (250ms sáng / 250ms tắt)
    traffic_set(1, LIGHT_GREEN);
    traffic_set(2, LIGHT_GREEN);
    button_clear_all();
}

/* Hàm lưu trực tiếp (Đỏ đã luôn bằng Xanh + Vàng nhờ tính toán real-time) */
static void apply_tuning_and_save(void)
{
    // Cập nhật giá trị vào biến hệ thống chính thức
    time_red   = temp_red;
    time_amber = temp_amber;
    time_green = temp_green;

    lcd_notice_code = 1; // Hiển thị "LUU OK!"
    SCH_Add_Task(global_clear_lcd_notice_oneshot, 150, 0); // Xóa thông báo sau 1.5s
    global_enter_auto_mode();
}

void fsm_tuning_run(void)
{
    switch (status) {
    case TUNE_GREEN:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_GREEN);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_green < 98) temp_green++;
            else temp_green = 1;
            temp_red = temp_green + temp_amber; // Tự động tính lại Đỏ ngay lập tức
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_green > 1) temp_green--;
            else temp_green = 98;
            temp_red = temp_green + temp_amber; // Tự động tính lại Đỏ ngay lập tức
        }
        if (isButtonPressed(BTN_SELECT)) {
            status = TUNE_AMBER; // Nhấn Select -> Chuyển sang chỉnh Vàng
            setTimer(2, 250);
            traffic_set(1, LIGHT_AMBER);
            traffic_set(2, LIGHT_AMBER);
        }
        if (isButtonPressed(BTN_MODE) || isButtonPressed(BTN_USER_B1)) {
            apply_tuning_and_save(); // Nhấn Mode hoặc User Button -> Lưu
        }
        break;

    case TUNE_AMBER:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_AMBER);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_amber < 10) temp_amber++;
            else temp_amber = 1;
            temp_red = temp_green + temp_amber; // Tự động tính lại Đỏ
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_amber > 1) temp_amber--;
            else temp_amber = 10;
            temp_red = temp_green + temp_amber; // Tự động tính lại Đỏ
        }
        if (isButtonPressed(BTN_SELECT)) {
            status = TUNE_GREEN; // Nhấn Select -> Vòng lại chỉnh Xanh
            setTimer(2, 250);
            traffic_set(1, LIGHT_GREEN);
            traffic_set(2, LIGHT_GREEN);
        }
        if (isButtonPressed(BTN_MODE) || isButtonPressed(BTN_USER_B1)) {
            apply_tuning_and_save(); // Nhấn Mode hoặc User Button -> Lưu
        }
        break;

    default:
        break;
    }
}
