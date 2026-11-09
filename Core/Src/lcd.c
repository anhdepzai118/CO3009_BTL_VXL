#include "lcd.h"
#include "global.h"
#include "dht20.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#define I2C_SCL_PORT    GPIOB
#define I2C_SCL_PIN     GPIO_PIN_8
#define I2C_SDA_PORT    GPIOB
#define I2C_SDA_PIN     GPIO_PIN_9

/* Địa chỉ 7-bit của LCD (mặc định 0x21 cho module ACLAB HCMUT, tự động quét khi khởi tạo) */
uint8_t lcd_i2c_addr = 0x21;
uint8_t found_i2c_addrs[4] = {0};
uint8_t found_i2c_count = 0;

static uint8_t i2c_initialized = 0;
static char prev_line1[17] = "";
static char prev_line2[17] = "";

static void i2c_short_delay(void)
{
    for (volatile int i = 0; i < 90; i++) {
        __NOP();
    }
}

static void lcd_wait_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * 12; i++) {
        __NOP();
    }
}

void i2c_bus_init(void)
{
    if (i2c_initialized) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, I2C_SCL_PIN | I2C_SDA_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    i2c_initialized = 1;
}

static void i2c_start(void)
{
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    i2c_short_delay();
}

static void i2c_stop(void)
{
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    i2c_short_delay();
}

static uint8_t i2c_write_byte(uint8_t byte)
{
    for (int b = 7; b >= 0; b--) {
        HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, (byte & (1 << b)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        i2c_short_delay();
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        i2c_short_delay();
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        i2c_short_delay();
    }
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    i2c_short_delay();
    uint8_t nack = (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET) ? 1 : 0;
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    i2c_short_delay();
    return nack;
}

static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t byte = 0;
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    for (int b = 7; b >= 0; b--) {
        i2c_short_delay();
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        i2c_short_delay();
        if (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET) {
            byte |= (1 << b);
        }
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, send_ack ? GPIO_PIN_RESET : GPIO_PIN_SET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    i2c_short_delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    return byte;
}

uint8_t i2c_bus_write(uint8_t addr7, const uint8_t *data, uint16_t len)
{
    i2c_start();
    if (i2c_write_byte((uint8_t)(addr7 << 1))) {
        i2c_stop();
        return 1;
    }
    for (uint16_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i])) {
            i2c_stop();
            return 1;
        }
    }
    i2c_stop();
    return 0;
}

uint8_t i2c_bus_read(uint8_t addr7, uint8_t *data, uint16_t len)
{
    i2c_start();
    if (i2c_write_byte((uint8_t)((addr7 << 1) | 0x01))) {
        i2c_stop();
        return 1;
    }
    for (uint16_t i = 0; i < len; i++) {
        data[i] = i2c_read_byte(i < (len - 1));
    }
    i2c_stop();
    return 0;
}

static void lcd_send_cmd(uint8_t cmd)
{
    if (lcd_i2c_addr == 0x3E) {
        /* Trường hợp LCD dùng IC điều khiển trực tiếp AiP31068L (0x3E) */
        uint8_t buf[2] = {0x80, cmd};
        i2c_bus_write(lcd_i2c_addr, buf, 2);
    } else {
        /* Trường hợp LCD chuẩn PCF8574 I2C Backpack (0x21, 0x27, 0x3F...) */
        uint8_t upper = cmd & 0xF0;
        uint8_t lower = (uint8_t)((cmd << 4) & 0xF0);
        uint8_t buf[4];
        buf[0] = upper | 0x0C; // EN=1, Backlight=1, RW=0, RS=0
        buf[1] = upper | 0x08; // EN=0, Backlight=1, RW=0, RS=0
        buf[2] = lower | 0x0C;
        buf[3] = lower | 0x08;
        i2c_bus_write(lcd_i2c_addr, buf, 4);
    }
}

static void lcd_send_data(uint8_t data)
{
    if (lcd_i2c_addr == 0x3E) {
        uint8_t buf[2] = {0x40, data};
        i2c_bus_write(lcd_i2c_addr, buf, 2);
    } else {
        uint8_t upper = data & 0xF0;
        uint8_t lower = (uint8_t)((data << 4) & 0xF0);
        uint8_t buf[4];
        buf[0] = upper | 0x0D; // EN=1, Backlight=1, RW=0, RS=1
        buf[1] = upper | 0x09; // EN=0, Backlight=1, RW=0, RS=1
        buf[2] = lower | 0x0D;
        buf[3] = lower | 0x09;
        i2c_bus_write(lcd_i2c_addr, buf, 4);
    }
}

void lcd_init(void)
{
    i2c_bus_init();
    lcd_wait_us(50000); // Chờ 50ms sau khi cấp nguồn cho LCD

    /* Tự động quét địa chỉ I2C của LCD (bỏ qua 0x38 của DHT20) */
    found_i2c_count = 0;
    for (uint8_t addr = 0x20; addr < 0x7F; addr++) {
        i2c_start();
        uint8_t nack = i2c_write_byte((uint8_t)(addr << 1));
        i2c_stop();
        if (nack == 0) {
            if (found_i2c_count < 4) {
                found_i2c_addrs[found_i2c_count++] = addr;
            }
            if (addr != DHT20_I2C_ADDR) {
                lcd_i2c_addr = addr;
            }
        }
    }

    /* Chuỗi khởi tạo chuẩn 4-bit cho HD44780 */
    lcd_send_cmd(0x33);
    lcd_wait_us(5000);
    lcd_send_cmd(0x32);
    lcd_wait_us(5000);
    lcd_send_cmd(0x28); // 4-bit mode, 2 dòng, font 5x8
    lcd_wait_us(1000);
    lcd_send_cmd(0x0C); // Bật hiển thị, tắt con trỏ
    lcd_wait_us(1000);
    lcd_send_cmd(0x06); // Tự động dịch con trỏ sang phải
    lcd_wait_us(1000);
    lcd_send_cmd(0x01); // Xóa màn hình
    lcd_wait_us(2000);

    prev_line1[0] = '\0';
    prev_line2[0] = '\0';
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    lcd_wait_us(2000);
    prev_line1[0] = '\0';
    prev_line2[0] = '\0';
}

void lcd_goto_xy(uint8_t row, uint8_t col)
{
    uint8_t pos = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(pos);
}

void lcd_send_string(const char *str)
{
    while (*str) {
        lcd_send_data((uint8_t)(*str++));
    }
}

/* Định dạng chuỗi đủ đúng 16 ký tự (bù khoảng trắng ở cuối để không cần gọi lcd_clear gây nhấp nháy) */
static void format_line16(char *dst16, const char *src)
{
    int i = 0;
    while (src[i] != '\0' && i < 16) {
        dst16[i] = src[i];
        i++;
    }
    while (i < 16) {
        dst16[i++] = ' ';
    }
    dst16[16] = '\0';
}

void lcd_update_task(void)
{
    char raw1[24] = {0};
    char raw2[24] = {0};
    char line1[17];
    char line2[17];

    int t_x10 = dht20_get_temp_x10();
    int h_x10 = dht20_get_humi_x10();
    int t_int = t_x10 / 10;
    int t_dec = (t_x10 >= 0) ? (t_x10 % 10) : (-t_x10 % 10);
    int h_int = h_x10 / 10;
    int h_dec = h_x10 % 10;

    /* 1. Xử lý nội dung Dòng 1 theo đúng trạng thái của cả 3 chế độ */
    switch (status) {
    /* --- CHẾ ĐỘ TỰ ĐỘNG: Hiển thị rõ Đỏ 1-Xanh 2 / Xanh 1-Đỏ 2 và số giây đếm ngược --- */
    case AUTO_RED1_GREEN2:
        snprintf(raw1, sizeof(raw1), "A D1:%02ds X2:%02ds", countdown1, countdown2);
        break;
    case AUTO_RED1_AMBER2:
        snprintf(raw1, sizeof(raw1), "A D1:%02ds V2:%02ds", countdown1, countdown2);
        break;
    case AUTO_GREEN1_RED2:
        snprintf(raw1, sizeof(raw1), "A X1:%02ds D2:%02ds", countdown1, countdown2);
        break;
    case AUTO_AMBER1_RED2:
        snprintf(raw1, sizeof(raw1), "A V1:%02ds D2:%02ds", countdown1, countdown2);
        break;

    /* --- CHẾ ĐỘ BẰNG TAY: Hiển thị rõ pha D1-X2 / X1-D2 và thời gian đếm --- */
    case MAN_RED1_GREEN2:
        snprintf(raw1, sizeof(raw1), "M D1-X2  t:%03ds", manual_elapsed);
        break;
    case MAN_RED1_AMBER2:
        snprintf(raw1, sizeof(raw1), "M D1:%02ds V2:%02ds", countdown1, countdown2);
        break;
    case MAN_GREEN1_RED2:
        snprintf(raw1, sizeof(raw1), "M X1-D2  t:%03ds", manual_elapsed);
        break;
    case MAN_AMBER1_RED2:
        snprintf(raw1, sizeof(raw1), "M V1:%02ds D2:%02ds", countdown1, countdown2);
        break;
    case MAN_BLINK_AMBER:
        snprintf(raw1, sizeof(raw1), "M V1-V2 CANH BAO");
        break;

    /* --- CHẾ ĐỘ CHỈNH THỜI GIAN: Hiển thị đèn đang chỉnh và bộ thông số --- */
    case TUNE_RED:
        snprintf(raw1, sizeof(raw1), "CHINH DO  : %02ds", temp_red);
        break;
    case TUNE_AMBER:
        snprintf(raw1, sizeof(raw1), "CHINH VANG: %02ds", temp_amber);
        break;
    case TUNE_GREEN:
        snprintf(raw1, sizeof(raw1), "CHINH XANH: %02ds", temp_green);
        break;

    default:
        snprintf(raw1, sizeof(raw1), "KHOI DONG HE...");
        break;
    }

    /* 2. Xử lý nội dung Dòng 2 */
    if (lcd_notice_code == 1) {
        snprintf(raw2, sizeof(raw2), "LUU OK! D=%02d X%02d", time_red, time_green);
    } else if (lcd_notice_code == 2) {
        snprintf(raw2, sizeof(raw2), "LOI: D != X + V!");
    } else if (lcd_notice_code == 3) {
        snprintf(raw2, sizeof(raw2), "RESET: D05X03V02");
    } else if (status == TUNE_RED || status == TUNE_AMBER || status == TUNE_GREEN) {
        snprintf(raw2, sizeof(raw2), "D:%02ds X:%02ds V:%02d", temp_red, temp_green, temp_amber);
    } else if (status == MAN_RED1_AMBER2) {
        snprintf(raw2, sizeof(raw2), "CHUYEN-> X1 - D2");
    } else if (status == MAN_AMBER1_RED2) {
        snprintf(raw2, sizeof(raw2), "CHUYEN-> D1 - X2");
    } else {
        snprintf(raw2, sizeof(raw2), "T:%02d.%dC  H:%02d.%d%%", t_int, t_dec, h_int, h_dec);
    }

    format_line16(line1, raw1);
    format_line16(line2, raw2);

    /* Chỉ gửi I2C khi nội dung dòng thực sự thay đổi để tối ưu tốc độ */
    if (strcmp(line1, prev_line1) != 0) {
        lcd_goto_xy(0, 0);
        lcd_send_string(line1);
        strcpy(prev_line1, line1);
    }
    if (strcmp(line2, prev_line2) != 0) {
        lcd_goto_xy(1, 0);
        lcd_send_string(line2);
        strcpy(prev_line2, line2);
    }
}
