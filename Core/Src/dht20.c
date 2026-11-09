#include "dht20.h"
#include "lcd.h"
#include "main.h"

static int temperature_x10 = 280; // Giá trị khởi tạo hợp lý (28.0 C) trước lần đo đầu tiên
static int humidity_x10    = 650; // Giá trị khởi tạo hợp lý (65.0 %)
static uint8_t dht_step    = 0;

void dht20_init(void)
{
    i2c_bus_init();
    dht_step = 0;
}

/*
 * Task đọc cảm biến DHT20 theo chuẩn Non-blocking (không dùng HAL_Delay):
 * - Lần gọi 1 (dht_step == 0): Gửi lệnh Trigger đo (0xAC, 0x33, 0x00).
 * - Lần gọi 2 (dht_step == 1, sau 1000ms > 80ms yêu cầu của datasheet):
 *   Đọc 7 byte dữ liệu và tính nhiệt độ / độ ẩm theo công thức 20-bit của DHT20.
 */
void dht20_read_task(void)
{
    if (dht_step == 0) {
        uint8_t cmd[3] = {0xAC, 0x33, 0x00};
        if (i2c_bus_write(DHT20_I2C_ADDR, cmd, 3) == 0) {
            dht_step = 1;
        }
    } else {
        uint8_t data[7] = {0};
        if (i2c_bus_read(DHT20_I2C_ADDR, data, 7) == 0) {
            /* Kiểm tra bit Busy (bit 7 của byte trạng thái data[0]) == 0 */
            if ((data[0] & 0x80) == 0x00) {
                uint32_t raw_humi = ((uint32_t)data[1] << 12) |
                                    ((uint32_t)data[2] << 4)  |
                                    ((uint32_t)(data[3] >> 4) & 0x0F);

                uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) |
                                    ((uint32_t)data[4] << 8)          |
                                    ((uint32_t)data[5]);

                /* Công thức chuẩn Datasheet DHT20 (nhân 10 để lấy 1 chữ số thập phân, tránh sai số float):
                 * RH_x10   = (raw_humi * 1000) / 1048576  = (raw_humi * 125) >> 17
                 * Temp_x10 = (raw_temp * 2000) / 1048576 - 500 = ((raw_temp * 125) >> 16) - 500
                 */
                humidity_x10    = (int)((raw_humi * 125UL) >> 17);
                temperature_x10 = (int)((raw_temp * 125UL) >> 16) - 500;
            }
        }
        dht_step = 0;
    }
}

int dht20_get_temp_x10(void)
{
    return temperature_x10;
}

int dht20_get_humi_x10(void)
{
    return humidity_x10;
}
