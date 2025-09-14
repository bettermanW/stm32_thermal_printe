//
// Created by 温厂长 on 2025/9/14.
//
#include "dr_led.h"



static void led_flash(const int ms) {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    HAL_Delay(ms);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(ms);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    HAL_Delay(ms);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(ms);
}

void led_run_state(const led_state state) {
    switch (state) {
        case WAIT:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 灭
            break;
        case CONNECTING:
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // 常亮
            break;
        case WRONG:
            led_flash(200);
            break;
        case PRINTING:
            led_flash(1000);
            break;
        default:
            break;
    }
}
