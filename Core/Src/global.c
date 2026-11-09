#include "global.h"
#include "software_timer.h"
#include "traffic_light.h"
#include "button.h"

int status = INIT;

/* Thời gian mặc định hợp lệ ban đầu: Đỏ = 5s, Xanh = 3s, Vàng = 2s (5 = 3 + 2) */
int time_red   = 5;
int time_amber = 2;
int time_green = 3;

int temp_red   = 5;
int temp_amber = 2;
int temp_green = 3;

int countdown1 = 5;
int countdown2 = 3;
int manual_elapsed = 0;

uint8_t lcd_notice_code = 0;

void global_enter_auto_mode(void)
{
    status = AUTO_RED1_GREEN2;
    countdown1 = time_red;
    countdown2 = time_green;

    traffic_set(1, LIGHT_RED);
    traffic_set(2, LIGHT_GREEN);

    /* Timer 0: đếm thời gian pha hiện tại; Timer 1: nhịp 1 giây giảm biến đếm ngược */
    setTimer(0, time_green * 1000);
    setTimer(1, 1000);
    button_clear_all();
}

void global_clear_lcd_notice_oneshot(void)
{
    lcd_notice_code = 0;
}
