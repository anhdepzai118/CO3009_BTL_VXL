#include "lcd.h"
#include "global.h"
#include "dht20.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/*
 * Trên chip STM32F103, chế độ GPIO_MODE_OUTPUT_OD tự động tắt trở kéo lên (Pull-up) nội.
 * Để đọc dữ liệu từ DHT20 chính xác (không bị đọc toàn bit 0 -> -50.0C, 00.0%),
 * khi thả chân SDA/SCL lên mức 1 ta chuyển nibble cấu hình trong GPIOB->CRH sang
 * Input Pull-Up (0x8 với ODR=1), khi kéo xuống mức 0 ta chuyển sang Output Open-Drain (0x7 với ODR=0).
 *   - PB8 (SCL): bit [3:0] của thanh ghi GPIOB->CRH
 *   - PB9 (SDA): bit [7:4] của thanh ghi GPIOB->CRH
 */
static inline void SCL_HIGH(void)
{
    GPIOB->BSRR = GPIO_PIN_8;
    GPIOB->CRH  = (GPIOB->CRH & ~(0xFU << 0)) | (0x3U << 0);
}

static inline void SCL_LOW(void)
{
    GPIOB->BRR = GPIO_PIN_8;
    GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 0)) | (0x7U << 0);
}

static inline void SDA_HIGH(void)
{
    GPIOB->BSRR = GPIO_PIN_9;
    GPIOB->CRH  = (GPIOB->CRH & ~(0xFU << 4)) | (0x8U << 4); // Input Pull-Up 3.3V nội
}

static inline void SDA_LOW(void)
{
    GPIOB->BRR = GPIO_PIN_9;
    GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 4)) | (0x7U << 4); // Output Open-Drain kéo xuống 0V
}

static inline uint8_t SDA_READ(void)
{
    return (GPIOB->IDR & GPIO_PIN_9) ? 1 : 0;
}

/* Địa chỉ 7-bit của LCD (mặc định 0x21 cho module ACLAB HCMUT) */
uint8_t lcd_i2c_addr = 0x21;
uint8_t found_i2c_addrs[4] = {0};
uint8_t found_i2c_count = 0;

static uint8_t i2c_initialized = 0;
static char prev_line1[17] = "";
static char prev_line2[17] = "";

/* Delay ngắn ~2.5us giúp cập nhật LCD cực nhanh (< 2.5ms), không làm trễ nhịp đếm giây */
static void i2c_short_delay(void)
{
    for (volatile int i = 0; i < 12; i++) {
        __NOP();
    }
}

static void lcd_wait_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * 8; i++) {
        __NOP();
    }
}

void i2c_bus_init(void)
{
    if (i2c_initialized) return;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    SCL_HIGH();
    SDA_HIGH();
    i2c_short_delay();

    i2c_initialized = 1;
}

static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    i2c_short_delay();
    SDA_LOW();
    i2c_short_delay();
    SCL_LOW();
    i2c_short_delay();
}

static void i2c_stop(void)
{
    SCL_LOW();
    SDA_LOW();
    i2c_short_delay();
    SCL_HIGH();
    i2c_short_delay();
    SDA_HIGH();
    i2c_short_delay();
}

static uint8_t i2c_write_byte(uint8_t byte)
{
    for (int b = 7; b >= 0; b--) {
        if (byte & (1 << b)) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        i2c_short_delay();
        SCL_HIGH();
        i2c_short_delay();
        SCL_LOW();
        i2c_short_delay();
    }
    SDA_HIGH(); // Thả SDA với trở kéo lên nội để đọc bit ACK từ Slave
    i2c_short_delay();
    SCL_HIGH();
    i2c_short_delay();
    uint8_t nack = SDA_READ();
    SCL_LOW();
    i2c_short_delay();
    return nack;
}

static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t byte = 0;
    SDA_HIGH(); // Chuyển SDA sang Input Pull-Up để đọc 8 bit dữ liệu từ DHT20
    for (int b = 7; b >= 0; b--) {
        i2c_short_delay();
        SCL_HIGH();
        i2c_short_delay();
        if (SDA_READ()) {
            byte |= (1 << b);
        }
        SCL_LOW();
        i2c_short_delay();
    }
    if (send_ack) {
        SDA_LOW();
    } else {
        SDA_HIGH();
    }
    i2c_short_delay();
    SCL_HIGH();
    i2c_short_delay();
    SCL_LOW();
    i2c_short_delay();
    SDA_HIGH();
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
        data[i] = i2c_read_byte((i < (len - 1)) ? 1 : 0);
    }
    i2c_stop();
    return 0;
}

static void lcd_send_cmd(uint8_t cmd)
{
    if (lcd_i2c_addr == 0x3E) {
        uint8_t buf[2] = {0x80, cmd};
        i2c_bus_write(lcd_i2c_addr, buf, 2);
    } else {
        uint8_t upper = cmd & 0xF0;
        uint8_t lower = (uint8_t)((cmd << 4) & 0xF0);
        uint8_t buf[4];
        buf[0] = upper | 0x0C;
        buf[1] = upper | 0x08;
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
        buf[0] = upper | 0x0D;
        buf[1] = upper | 0x09;
        buf[2] = lower | 0x0D;
        buf[3] = lower | 0x09;
        i2c_bus_write(lcd_i2c_addr, buf, 4);
    }
}

void lcd_init(void)
{
    i2c_bus_init();
    lcd_wait_us(50000);

    /* Quét địa chỉ I2C của LCD (bỏ qua 0x38 của DHT20) */
    found_i2c_count = 0;
    for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
        if (addr == DHT20_I2C_ADDR) {
            if (found_i2c_count < 4) found_i2c_addrs[found_i2c_count++] = addr;
            continue;
        }
        i2c_start();
        uint8_t nack = i2c_write_byte((uint8_t)(addr << 1));
        i2c_stop();
        if (nack == 0) {
            if (found_i2c_count < 4) {
                found_i2c_addrs[found_i2c_count++] = addr;
            }
            lcd_i2c_addr = addr;
        }
    }

    lcd_send_cmd(0x33);
    lcd_wait_us(5000);
    lcd_send_cmd(0x32);
    lcd_wait_us(5000);
    lcd_send_cmd(0x28);
    lcd_wait_us(1000);
    lcd_send_cmd(0x0C);
    lcd_wait_us(1000);
    lcd_send_cmd(0x06);
    lcd_wait_us(1000);
    lcd_send_cmd(0x01);
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
    int h_dec = (h_x10 >= 0) ? (h_x10 % 10) : 0;

    /* 1. Xử lý nội dung Dòng 1 theo đúng trạng thái của cả 3 chế độ */
    switch (status) {
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
        snprintf(raw2, sizeof(raw2), "T:%02d.%dC H:%02d.%d%%", t_int, t_dec, h_int, h_dec);
    }

    format_line16(line1, raw1);
    format_line16(line2, raw2);

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
