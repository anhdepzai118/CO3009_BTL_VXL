#include "traffic_light.h"
#include "main.h"

#ifndef TL1_A_Pin
#define TL1_A_Pin         GPIO_PIN_3
#define TL1_A_GPIO_Port   GPIOB
#define TL1_B_Pin         GPIO_PIN_5
#define TL1_B_GPIO_Port   GPIOB
#define TL2_A_Pin         GPIO_PIN_4
#define TL2_A_GPIO_Port   GPIOB
#define TL2_B_Pin         GPIO_PIN_10
#define TL2_B_GPIO_Port   GPIOB
#endif

static uint8_t blink_toggle_state = 0;

/*
 * Bảng tổ hợp tín hiệu chuẩn trên PCB module đèn (AB - LED):
 *   (TLx_A, TLx_B) = (0, 0) -> LIGHT_OFF   (00 - OFF : Tắt)
 *   (TLx_A, TLx_B) = (0, 1) -> LIGHT_AMBER (01 - YEL : Đèn Vàng)
 *   (TLx_A, TLx_B) = (1, 0) -> LIGHT_GREEN (10 - GRE : Đèn Xanh)
 *   (TLx_A, TLx_B) = (1, 1) -> LIGHT_RED   (11 - RED : Đèn Đỏ)
 */
static void decode_color_pins(TrafficColor_t color, GPIO_PinState *pinA, GPIO_PinState *pinB)
{
    switch (color) {
    case LIGHT_AMBER:
        *pinA = GPIO_PIN_RESET; // A = 0
        *pinB = GPIO_PIN_SET;   // B = 1 -> 01: Vàng (YEL)
        break;
    case LIGHT_GREEN:
        *pinA = GPIO_PIN_SET;   // A = 1
        *pinB = GPIO_PIN_RESET; // B = 0 -> 10: Xanh (GRE)
        break;
    case LIGHT_RED:
        *pinA = GPIO_PIN_SET;   // A = 1
        *pinB = GPIO_PIN_SET;   // B = 1 -> 11: Đỏ (RED)
        break;
    case LIGHT_OFF:
    default:
        *pinA = GPIO_PIN_RESET;
        *pinB = GPIO_PIN_RESET;
        break;
    }
}

void traffic_init(void)
{
    traffic_set(1, LIGHT_OFF);
    traffic_set(2, LIGHT_OFF);
    blink_toggle_state = 0;
}

void traffic_set(uint8_t road, TrafficColor_t color)
{
    GPIO_PinState sigA, sigB;
    decode_color_pins(color, &sigA, &sigB);

    if (road == 1) {
        HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, sigA);
        HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, sigB);
    } else if (road == 2) {
        HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, sigA);
        HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, sigB);
    }
}

void traffic_toggle_both(TrafficColor_t color)
{
    blink_toggle_state = !blink_toggle_state;
    if (blink_toggle_state) {
        traffic_set(1, color);
        traffic_set(2, color);
    } else {
        traffic_set(1, LIGHT_OFF);
        traffic_set(2, LIGHT_OFF);
    }
}
