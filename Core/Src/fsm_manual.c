#include "fsm_manual.h"
#include "fsm_tuning.h"
#include "global.h"
#include "software_timer.h"
#include "button.h"
#include "traffic_light.h"

void fsm_manual_run(void)
{
    switch (status) {
    case MAN_RED1_GREEN2:
        traffic_set(1, LIGHT_RED);
        traffic_set(2, LIGHT_GREEN);

        /* Đếm thời gian duy trì pha hiện tại (giây) để dễ theo dõi trên LCD */
        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (manual_elapsed < 999) manual_elapsed++;
        }

        /* Nhấn BTN_SELECT (Nút 2 - D8): Chuyển sang pha đệm Vàng 2 an toàn 2 giây trước khi đổi sang Xanh 1 - Đỏ 2 */
        if (isButtonPressed(BTN_SELECT)) {
            status = MAN_RED1_AMBER2;
            countdown1 = 2;
            countdown2 = 2;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_AMBER);
            setTimer(0, 2000);
            setTimer(1, 1000);
        } else if (isButtonPressed(BTN_INC)) {
            /* Nhấn BTN_INC (Nút 3 - D9): Bật chế độ nhấp nháy Vàng cảnh báo 2 tuyến */
            status = MAN_BLINK_AMBER;
            setTimer(2, 500);
        } else if (isButtonPressed(BTN_MODE)) {
            /* Nhấn BTN_MODE (Nút 1 - D7): Chuyển sang chế độ Chỉnh thời gian (Tuning) */
            fsm_tuning_enter();
        } else if (isButtonPressed(BTN_USER_B1)) {
            global_enter_auto_mode();
        }
        break;

    case MAN_RED1_AMBER2:
        traffic_set(1, LIGHT_RED);
        traffic_set(2, LIGHT_AMBER);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết 2 giây đèn Vàng an toàn -> sang Xanh 1 - Đỏ 2 */
        if (timer_flag[0] == 1) {
            status = MAN_GREEN1_RED2;
            manual_elapsed = 0;
            traffic_set(1, LIGHT_GREEN);
            traffic_set(2, LIGHT_RED);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            fsm_tuning_enter();
        } else if (isButtonPressed(BTN_USER_B1)) {
            global_enter_auto_mode();
        }
        break;

    case MAN_GREEN1_RED2:
        traffic_set(1, LIGHT_GREEN);
        traffic_set(2, LIGHT_RED);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (manual_elapsed < 999) manual_elapsed++;
        }

        /* Nhấn BTN_SELECT (Nút 2 - D8): Chuyển sang pha đệm Vàng 1 an toàn 2 giây trước khi đổi sang Đỏ 1 - Xanh 2 */
        if (isButtonPressed(BTN_SELECT)) {
            status = MAN_AMBER1_RED2;
            countdown1 = 2;
            countdown2 = 2;
            traffic_set(1, LIGHT_AMBER);
            traffic_set(2, LIGHT_RED);
            setTimer(0, 2000);
            setTimer(1, 1000);
        } else if (isButtonPressed(BTN_INC)) {
            status = MAN_BLINK_AMBER;
            setTimer(2, 500);
        } else if (isButtonPressed(BTN_MODE)) {
            fsm_tuning_enter();
        } else if (isButtonPressed(BTN_USER_B1)) {
            global_enter_auto_mode();
        }
        break;

    case MAN_AMBER1_RED2:
        traffic_set(1, LIGHT_AMBER);
        traffic_set(2, LIGHT_RED);

        if (timer_flag[1] == 1) {
            setTimer(1, 1000);
            if (countdown1 > 1) countdown1--;
            if (countdown2 > 1) countdown2--;
        }

        /* Hết 2 giây đèn Vàng an toàn -> sang Đỏ 1 - Xanh 2 */
        if (timer_flag[0] == 1) {
            status = MAN_RED1_GREEN2;
            manual_elapsed = 0;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_GREEN);
            setTimer(1, 1000);
        }

        if (isButtonPressed(BTN_MODE)) {
            fsm_tuning_enter();
        } else if (isButtonPressed(BTN_USER_B1)) {
            global_enter_auto_mode();
        }
        break;

    case MAN_BLINK_AMBER:
        if (timer_flag[2] == 1) {
            setTimer(2, 500);
            traffic_toggle_both(LIGHT_AMBER);
        }

        if (isButtonPressed(BTN_SELECT) || isButtonPressed(BTN_INC)) {
            status = MAN_RED1_GREEN2;
            manual_elapsed = 0;
            traffic_set(1, LIGHT_RED);
            traffic_set(2, LIGHT_GREEN);
            setTimer(1, 1000);
        } else if (isButtonPressed(BTN_MODE)) {
            fsm_tuning_enter();
        } else if (isButtonPressed(BTN_USER_B1)) {
            global_enter_auto_mode();
        }
        break;

    default:
        break;
    }
}
