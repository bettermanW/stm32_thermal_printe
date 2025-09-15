//
// Created by 温厂长 on 2025/9/14.
//
#include "dr_led.h"



static void led_flash(const uint8_t count, const int ms) {
    for (uint8_t i = 0; i < count; ++i) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(ms);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(ms);
    }
}

void led_run_state(const led_state_e state) {
    switch (state) {
        case LED_CONNECTED:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // 亮
            break;
        case LED_DISCONNECTED:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 灭
            break;
        case LED_WARONG:
            led_flash(3, 200);  //快速闪烁3次，间隔200ms
            break;
        case LED_PRINT_START:   // 单次闪烁（亮-灭）
            led_flash(1, 200);
            break;
        case LED_BLE_INIT:
            led_flash(1, 10);   // 极快速闪烁一次
        default:
            break;
    }
}
