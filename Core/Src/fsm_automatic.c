#include "fsm_automatic.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"
#include "scheduler.h"

/* Chuyển an toàn từ chế độ Tự động sang chế độ Bằng tay */
static void transition_auto_to_manual(void)
{
    clearTimer(0);
    manual_elapsed = 0;
    setTimer(1, 1000);

    /* Kế thừa luồng giao thông hiện tại để không cắt ngang đột ngột và không bao giờ 2 tuyến cùng Xanh */
    if (status == AUTO_RED1_GREEN2 || status == AUTO_RED1_AMBER2) {
        status = MAN_RED1_GREEN2;
        traffic_set(1, LIGHT_RED);
        traffic_set(2, LIGHT_GREEN);
    } else {
        status = MAN_GREEN1_RED2;
        traffic_set(1, LIGHT_GREEN);
        traffic_set(2, LIGHT_RED);
    }

    button_clear_all();
}

/* Khôi phục thời gian mặc định (Đỏ 5s, Xanh 3s, Vàng 2s) khi nhấn nút USER (PC13) */
static void reset_default_times(void)
{
    time_red   = 5;
    time_green = 3;
    time_amber = 2;
    lcd_notice_code = 3;
    SCH_Add_Task(global_clear_lcd_notice_oneshot, 150, 0); // One-shot task xóa thông báo sau 1.5s
    global_enter_auto_mode();
}

void fsm_automatic_run(void)
{
    switch (status) {
    case INIT:
        global_enter_auto_mode();
        break;

    case AUTO_RED1_GREEN2:
        traffic_set(1, LIGHT_RED);
        traffic_set(2, LIGHT_GREEN);

        /* Giảm bộ đếm ngược mỗi 1 giây */
        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết thời gian Xanh 2 -> chuyển sang Đỏ 1 - Vàng 2 */
        if (timer_flag[0] == 1) {
            status = AUTO_RED1_AMBER2;
            countdown1 = time_amber;
            countdown2 = time_amber;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_AMBER);
            setTimer(0, time_amber * 1000);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            transition_auto_to_manual();
        } else if (isButtonPressed(BTN_USER_B1)) {
            reset_default_times();
        }
        break;

    case AUTO_RED1_AMBER2:
        traffic_set(1, LIGHT_RED);
        traffic_set(2, LIGHT_AMBER);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết thời gian Vàng 2 -> đổi chiều sang Xanh 1 - Đỏ 2 */
        if (timer_flag[0] == 1) {
            status = AUTO_GREEN1_RED2;
            countdown1 = time_green;
            countdown2 = time_red;
            traffic_set(1, LIGHT_GREEN);
            traffic_set(2, LIGHT_RED);
            setTimer(0, time_green * 1000);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            transition_auto_to_manual();
        } else if (isButtonPressed(BTN_USER_B1)) {
            reset_default_times();
        }
        break;

    case AUTO_GREEN1_RED2:
        traffic_set(1, LIGHT_GREEN);
        traffic_set(2, LIGHT_RED);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết thời gian Xanh 1 -> chuyển sang Vàng 1 - Đỏ 2 */
        if (timer_flag[0] == 1) {
            status = AUTO_AMBER1_RED2;
            countdown1 = time_amber;
            countdown2 = time_amber;
            traffic_set(1, LIGHT_AMBER);
            traffic_set(2, LIGHT_RED);
            setTimer(0, time_amber * 1000);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            transition_auto_to_manual();
        } else if (isButtonPressed(BTN_USER_B1)) {
            reset_default_times();
        }
        break;

    case AUTO_AMBER1_RED2:
        traffic_set(1, LIGHT_AMBER);
        traffic_set(2, LIGHT_RED);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết thời gian Vàng 1 -> quay lại đầu chu kỳ Đỏ 1 - Xanh 2 */
        if (timer_flag[0] == 1) {
            status = AUTO_RED1_GREEN2;
            countdown1 = time_red;
            countdown2 = time_green;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_GREEN);
            setTimer(0, time_green * 1000);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            transition_auto_to_manual();
        } else if (isButtonPressed(BTN_USER_B1)) {
            reset_default_times();
        }
        break;

    default:
        break;
    }
}
