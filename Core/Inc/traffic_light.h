#ifndef INC_TRAFFIC_LIGHT_H_
#define INC_TRAFFIC_LIGHT_H_

#include <stdint.h>

/* Các trạng thái màu đèn của mỗi tuyến */
typedef enum {
    LIGHT_OFF   = 0,
    LIGHT_GREEN = 1,
    LIGHT_AMBER = 2,
    LIGHT_RED   = 3
} TrafficColor_t;

/* Khởi tạo và tắt toàn bộ đèn lúc đầu */
void traffic_init(void);

/* Đặt màu đèn cho tuyến road (1 = Tuyến 1 D3-D4, 2 = Tuyến 2 D5-D6) */
void traffic_set(uint8_t road, TrafficColor_t color);

/* Đảo trạng thái chớp tắt của cả 2 tuyến đối với màu đèn đang chọn (dùng trong Tuning & Cảnh báo) */
void traffic_toggle_both(TrafficColor_t color);

#endif /* INC_TRAFFIC_LIGHT_H_ */
