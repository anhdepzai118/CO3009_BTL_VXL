#include "software_timer.h"

int timer_flag[MAX_SW_TIMERS] = {0};
static int timer_counter[MAX_SW_TIMERS] = {0};

void setTimer(int index, int duration_ms)
{
    if (index < 0 || index >= MAX_SW_TIMERS) return;
    timer_counter[index] = duration_ms / TIMER_TICK_MS;
    timer_flag[index] = 0;
    if (timer_counter[index] <= 0) {
        timer_flag[index] = 1;
    }
}

void clearTimer(int index)
{
    if (index < 0 || index >= MAX_SW_TIMERS) return;
    timer_counter[index] = 0;
    timer_flag[index] = 0;
}

void timer_run(void)
{
    for (int i = 0; i < MAX_SW_TIMERS; i++) {
        if (timer_counter[i] > 0) {
            timer_counter[i]--;
            if (timer_counter[i] <= 0) {
                timer_flag[i] = 1;
            }
        }
    }
}
