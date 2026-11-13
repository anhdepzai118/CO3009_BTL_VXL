#include "dht20.h"
#include "lcd.h"
#include "main.h"

static int temperature_x10 = 285; // Giá trị khởi tạo hợp lý (28.5 C)
static int humidity_x10    = 650; // Giá trị khởi tạo hợp lý (65.0 %)
static uint8_t dht_step    = 0;

void dht20_init(void)
{
    i2c_bus_init();
    dht_step = 0;
}

/*
 * Task đọc cảm biến DHT20 (Non-blocking) trên bus I2C chuẩn (PB8=SCL, PB9=SDA):
 * - Bước 0: Gửi lệnh khởi tạo hiệu chuẩn (0xBE, 0x08, 0x00) lần đầu.
 * - Bước 1: Gửi lệnh bắt đầu đo (0xAC, 0x33, 0x00).
 * - Bước 2: Đọc 7 byte kết quả ở chu kỳ kế tiếp, kiểm tra dữ liệu khác 0 trước khi cập nhật.
 */
void dht20_read_task(void)
{
    if (dht_step == 0) {
        uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
        i2c_bus_write(DHT20_I2C_ADDR, init_cmd, 3);
        dht_step = 1;
    } else if (dht_step == 1) {
        uint8_t trig_cmd[3] = {0xAC, 0x33, 0x00};
        if (i2c_bus_write(DHT20_I2C_ADDR, trig_cmd, 3) == 0) {
            dht_step = 2;
        }
    } else {
        uint8_t data[7] = {0};
        if (i2c_bus_read(DHT20_I2C_ADDR, data, 7) == 0) {
            /* Kiểm tra cảm biến đã đo xong (bit 7 == 0) và khung dữ liệu không bị rỗng */
            if (data[0] != 0x00 && data[0] != 0xFF && (data[0] & 0x80) == 0x00) {
                uint32_t raw_humi = ((uint32_t)data[1] << 12) |
                                    ((uint32_t)data[2] << 4)  |
                                    ((uint32_t)(data[3] >> 4) & 0x0F);

                uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) |
                                    ((uint32_t)data[4] << 8)          |
                                    ((uint32_t)data[5]);

                if (raw_humi > 0 && raw_temp > 0 && raw_humi < 0xFFFFF && raw_temp < 0xFFFFF) {
                    int new_humi = (int)((raw_humi * 125UL) >> 17);
                    int new_temp = (int)((raw_temp * 125UL) >> 16) - 500;

                    if (new_temp >= -100 && new_temp <= 800 && new_humi >= 10 && new_humi <= 999) {
                        humidity_x10    = new_humi;
                        temperature_x10 = new_temp;
                    }
                }
            }
        }
        dht_step = 1;
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
