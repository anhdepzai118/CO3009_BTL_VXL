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

    status = TUNE_RED;
    clearTimer(0);
    clearTimer(1);
    setTimer(2, 250); // Chớp tắt 2 Hz (250ms sáng / 250ms tắt)
    traffic_set(1, LIGHT_RED);
    traffic_set(2, LIGHT_RED);
    button_clear_all();
}

/* Kiểm tra ràng buộc hợp lệ (Đỏ == Xanh + Vàng) và chuyển về chế độ Tự động */
static void validate_and_apply_tuning(void)
{
    if (temp_red == (temp_green + temp_amber) &&
        temp_red >= 2 && temp_green >= 1 && temp_amber >= 1) {
        /* Hợp lệ: Lưu cấu hình mới và khởi động lại chu kỳ Tự động */
        time_red   = temp_red;
        time_amber = temp_amber;
        time_green = temp_green;

        lcd_notice_code = 1; // Hiển thị "LUU OK!"
        SCH_Add_Task(global_clear_lcd_notice_oneshot, 150, 0); // One-shot task xóa thông báo sau 1.5s
        global_enter_auto_mode();
    } else if (lcd_notice_code == 2) {
        /* Nếu đang hiện cảnh báo lỗi mà người dùng nhấn BTN_MODE lần 2 -> tự động cân bằng D = X + V và lưu */
        temp_red = temp_green + temp_amber;
        if (temp_red > 99) {
            temp_red = 99;
            temp_green = temp_red - temp_amber;
        }
        time_red   = temp_red;
        time_amber = temp_amber;
        time_green = temp_green;

        lcd_notice_code = 1;
        SCH_Add_Task(global_clear_lcd_notice_oneshot, 150, 0);
        global_enter_auto_mode();
    } else {
        /* Không hợp lệ: Báo lỗi trên LCD bằng One-shot Task trong 2 giây và giữ ở chế độ Tuning */
        lcd_notice_code = 2; // "LOI: D != X + V!"
        SCH_Add_Task(global_clear_lcd_notice_oneshot, 200, 0);
    }
}

/* Tự động cân bằng Đỏ = Xanh + Vàng và lưu ngay khi nhấn nút USER (PC13) */
static void auto_balance_and_save(void)
{
    temp_red = temp_green + temp_amber;
    if (temp_red > 99) {
        temp_red = 99;
        temp_green = temp_red - temp_amber;
    }
    time_red   = temp_red;
    time_amber = temp_amber;
    time_green = temp_green;

    lcd_notice_code = 1;
    SCH_Add_Task(global_clear_lcd_notice_oneshot, 150, 0);
    global_enter_auto_mode();
}

void fsm_tuning_run(void)
{
    switch (status) {
    case TUNE_RED:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_RED);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_red < 99) temp_red++;
            else temp_red = 2;
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_red > 2) temp_red--;
            else temp_red = 99;
        }
        if (isButtonPressed(BTN_SELECT)) {
            status = TUNE_AMBER;
            setTimer(2, 250);
            traffic_set(1, LIGHT_AMBER);
            traffic_set(2, LIGHT_AMBER);
        }
        if (isButtonPressed(BTN_MODE)) {
            validate_and_apply_tuning();
        } else if (isButtonPressed(BTN_USER_B1)) {
            auto_balance_and_save();
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
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_amber > 1) temp_amber--;
            else temp_amber = 10;
        }
        if (isButtonPressed(BTN_SELECT)) {
            status = TUNE_GREEN;
            setTimer(2, 250);
            traffic_set(1, LIGHT_GREEN);
            traffic_set(2, LIGHT_GREEN);
        }
        if (isButtonPressed(BTN_MODE)) {
            validate_and_apply_tuning();
        } else if (isButtonPressed(BTN_USER_B1)) {
            auto_balance_and_save();
        }
        break;

    case TUNE_GREEN:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_GREEN);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_green < 98) temp_green++;
            else temp_green = 1;
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_green > 1) temp_green--;
            else temp_green = 98;
        }
        if (isButtonPressed(BTN_SELECT)) {
            status = TUNE_RED;
            setTimer(2, 250);
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_RED);
        }
        if (isButtonPressed(BTN_MODE)) {
            validate_and_apply_tuning();
        } else if (isButtonPressed(BTN_USER_B1)) {
            auto_balance_and_save();
        }
        break;

    default:
        break;
    }
}
