#include "button.h"
#include "main.h"

#ifndef BTN_1_Pin
#define BTN_1_Pin         GPIO_PIN_8
#define BTN_1_GPIO_Port   GPIOA
#define BTN_2_Pin         GPIO_PIN_9
#define BTN_2_GPIO_Port   GPIOA
#define BTN_3_Pin         GPIO_PIN_7
#define BTN_3_GPIO_Port   GPIOC
#define BTN_4_Pin         GPIO_PIN_6
#define BTN_4_GPIO_Port   GPIOB
#endif

static GPIO_TypeDef* const btn_ports[NUM_BUTTONS] = {
    BTN_1_GPIO_Port,
    BTN_2_GPIO_Port,
    BTN_3_GPIO_Port,
    BTN_4_GPIO_Port,
    B1_GPIO_Port
};

static const uint16_t btn_pins[NUM_BUTTONS] = {
    BTN_1_Pin,
    BTN_2_Pin,
    BTN_3_Pin,
    BTN_4_Pin,
    B1_Pin
};

/* Mức logic khi KHÔNG nhấn của từng nút (tự động hiệu chuẩn lúc khởi động) */
static GPIO_PinState idle_level[NUM_BUTTONS];

/* 3 bộ đệm lọc chống dội phím (Debounce) */
static GPIO_PinState key_reg0[NUM_BUTTONS];
static GPIO_PinState key_reg1[NUM_BUTTONS];
static GPIO_PinState key_reg2[NUM_BUTTONS];
static GPIO_PinState key_stable[NUM_BUTTONS];

/* Bộ đếm nhấn giữ (Long press) và cờ sự kiện */
static int timer_for_key_press[NUM_BUTTONS];
static int button_flag[NUM_BUTTONS];
static int button_long_flag[NUM_BUTTONS];

void button_init(void)
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        GPIO_PinState init_val = HAL_GPIO_ReadPin(btn_ports[i], btn_pins[i]);
        idle_level[i] = init_val;
        key_reg0[i]   = init_val;
        key_reg1[i]   = init_val;
        key_reg2[i]   = init_val;
        key_stable[i] = init_val;
        timer_for_key_press[i] = 100; // 100 * 10ms = 1 giây
        button_flag[i] = 0;
        button_long_flag[i] = 0;
    }
}

void button_reading(void)
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        key_reg2[i] = key_reg1[i];
        key_reg1[i] = key_reg0[i];
        key_reg0[i] = HAL_GPIO_ReadPin(btn_ports[i], btn_pins[i]);

        /* Khi 3 lần đọc liên tiếp (30ms) có cùng giá trị -> phím đã ổn định */
        if ((key_reg0[i] == key_reg1[i]) && (key_reg1[i] == key_reg2[i])) {
            if (key_stable[i] != key_reg2[i]) {
                key_stable[i] = key_reg2[i];

                if (key_stable[i] != idle_level[i]) {
                    /* Phát hiện sườn nhấn phím */
                    button_flag[i] = 1;
                    timer_for_key_press[i] = 100; // Đợi 1.0s trước khi tính là nhấn giữ
                }
            } else {
                /* Phím vẫn đang giữ nguyên trạng thái nhấn */
                if (key_stable[i] != idle_level[i]) {
                    if (timer_for_key_press[i] > 0) {
                        timer_for_key_press[i]--;
                        if (timer_for_key_press[i] == 0) {
                            button_flag[i] = 1;
                            button_long_flag[i] = 1;
                            timer_for_key_press[i] = 25; // Tự động lặp lại mỗi 250ms khi giữ phím
                        }
                    }
                }
            }
        }
    }
}

int isButtonPressed(int index)
{
    if (index < 0 || index >= NUM_BUTTONS) return 0;
    if (button_flag[index] == 1) {
        button_flag[index] = 0;
        return 1;
    }
    return 0;
}

int isButtonLongPressed(int index)
{
    if (index < 0 || index >= NUM_BUTTONS) return 0;
    if (button_long_flag[index] == 1) {
        button_long_flag[index] = 0;
        return 1;
    }
    return 0;
}

void button_clear_all(void)
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_flag[i] = 0;
        button_long_flag[i] = 0;
    }
}
