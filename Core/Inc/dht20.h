#ifndef INC_DHT20_H_
#define INC_DHT20_H_

#include <stdint.h>

#define DHT20_I2C_ADDR  0x38

/* Khởi tạo cảm biến DHT20 trên bus I2C (PB8: SCL, PB9: SDA) */
void dht20_init(void);

/* Task đọc cảm biến DHT20 không chặn (Non-blocking), gọi định kỳ mỗi 1000ms từ Scheduler */
void dht20_read_task(void);

/* Lấy giá trị nhiệt độ x10 (ví dụ: 285 tương ứng 28.5 độ C) */
int dht20_get_temp_x10(void);

/* Lấy giá trị độ ẩm x10 (ví dụ: 652 tương ứng 65.2 %RH) */
int dht20_get_humi_x10(void);

#endif /* INC_DHT20_H_ */
