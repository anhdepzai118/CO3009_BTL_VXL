#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#define MAX_SW_TIMERS   5
#define TIMER_TICK_MS   10  // Chu kỳ gọi timer_run() là 10 ms

extern int timer_flag[MAX_SW_TIMERS];

/* Đặt thời gian đếm ngược cho bộ timer index (đơn vị: mili-giây, ví dụ 2000 = 2s) */
void setTimer(int index, int duration_ms);

/* Xóa bộ đếm timer */
void clearTimer(int index);

/* Hàm cập nhật các bộ đếm phần mềm (được gọi như một Task chu kỳ 10ms của Scheduler) */
void timer_run(void);

#endif /* INC_SOFTWARE_TIMER_H_ */
