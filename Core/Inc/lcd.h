#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdint.h>

/* Các hàm giao tiếp bus I2C dùng chung cho LCD và DHT20 trên chân PB8 (SCL), PB9 (SDA) */
void    i2c_bus_init(void);
uint8_t i2c_bus_write(uint8_t addr7, const uint8_t *data, uint16_t len);
uint8_t i2c_bus_read(uint8_t addr7, uint8_t *data, uint16_t len);

/* Các hàm điều khiển màn hình LCD 16x2 */
void lcd_init(void);
void lcd_clear(void);
void lcd_goto_xy(uint8_t row, uint8_t col);
void lcd_send_string(const char *str);

/* Task cập nhật hiển thị màn hình LCD cho tất cả các chế độ (gọi định kỳ từ Scheduler) */
void lcd_update_task(void);

#endif /* INC_LCD_H_ */
