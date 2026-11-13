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

    /* Vào thẳng chế độ chỉnh Xanh trước, Đỏ tự động = Xanh + Vàng */
    status = TUNE_GREEN;
    clearTimer(0);
    clearTimer(1);
    setTimer(2, 250); // Chớp tắt 2 Hz (250ms sáng / 250ms tắt)
    traffic_set(1, LIGHT_GREEN);
    traffic_set(2, LIGHT_GREEN);
    button_clear_all();
}

/* Chuyển từ Chỉnh Xanh sang Chỉnh Vàng */
static void switch_to_tune_amber(void)
{
    status = TUNE_AMBER;
    setTimer(2, 250);
    traffic_set(1, LIGHT_AMBER);
    traffic_set(2, LIGHT_AMBER);
    button_clear_all();
}

/* Chuyển từ Chỉnh Vàng vòng lại Chỉnh Xanh */
static void switch_to_tune_green(void)
{
    status = TUNE_GREEN;
    setTimer(2, 250);
    traffic_set(1, LIGHT_GREEN);
    traffic_set(2, LIGHT_GREEN);
    button_clear_all();
}

/* Lưu trực tiếp (Đỏ luôn bằng Xanh + Vàng) và quay về chế độ Tự động */
static void apply_tuning_and_save(void)
{
    temp_red   = temp_green + temp_amber;
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
    /* Bước 1 của Tuning: Chỉnh thời gian đèn XANH (Đỏ tự động = Xanh + Vàng) */
    case TUNE_GREEN:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_GREEN);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_green < 89) temp_green++;
            else temp_green = 1;
            temp_red = temp_green + temp_amber;
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_green > 1) temp_green--;
            else temp_green = 89;
            temp_red = temp_green + temp_amber;
        }

        /* Nhấn BTN_MODE (Nút 1) hoặc BTN_SELECT (Nút 2) đều chuyển tiếp sang Chỉnh VÀNG */
        if (isButtonPressed(BTN_MODE) || isButtonPressed(BTN_SELECT)) {
            switch_to_tune_amber();
        } else if (isButtonPressed(BTN_USER_B1)) {
            apply_tuning_and_save();
        }
        break;

    /* Bước 2 của Tuning: Chỉnh thời gian đèn VÀNG (Đỏ tự động = Xanh + Vàng) */
    case TUNE_AMBER:
        if (timer_flag[2] == 1) {
            setTimer(2, 250);
            traffic_toggle_both(LIGHT_AMBER);
        }

        if (isButtonPressed(BTN_INC)) {
            if (temp_amber < 10) temp_amber++;
            else temp_amber = 1;
            temp_red = temp_green + temp_amber;
        }
        if (isButtonPressed(BTN_DEC)) {
            if (temp_amber > 1) temp_amber--;
            else temp_amber = 10;
            temp_red = temp_green + temp_amber;
        }

        /* Nhấn BTN_SELECT (Nút 2) để quay lại Chỉnh XANH nếu muốn sửa tiếp */
        if (isButtonPressed(BTN_SELECT)) {
            switch_to_tune_green();
        }
        /* Nhấn BTN_MODE (Nút 1) hoặc nút USER sau khi chỉnh Vàng xong -> Lưu và về AUTO */
        else if (isButtonPressed(BTN_MODE) || isButtonPressed(BTN_USER_B1)) {
            apply_tuning_and_save();
        }
        break;

    default:
        break;
    }
}
