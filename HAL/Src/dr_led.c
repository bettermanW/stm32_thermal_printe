//
// Created by 温厂长 on 2025/9/14.
//
#include "dr_led.h"



void led_flash(const uint8_t count, const uint8_t ms) {
    for (uint8_t i = 0; i < count; ++i) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(ms);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(ms);
    }
}

void led_on() {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  //亮
}

void led_off() {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);    // 灭
}


