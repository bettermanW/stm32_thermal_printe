//
// Created by 温厂长 on 2025/9/12.
//
#include "dr_btn.h"

#include "main.h"
#include "stm32f1xx_hal_gpio.h"


#define SHORT_PRESS_TIME 1000 // 长按时间阈值

#define READ_BTN HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) // 按下低电平

// 状态变量
// 空闲状态 → 按下检测 → 按下确认 → 等待释放 → 处理释放
bool keyIsPressed = false; // 按键状态
unsigned long clickTime = 0;
bool longPressIsStart = false;

/**
 * @brief 按键检测
 */
void key_check_run(void) {
    // 阶段1：按下检测和消抖
    if (!keyIsPressed) {
        if (READ_BTN == false) { // 检测到可能按下
            HAL_Delay(10); // 消抖延迟
            if (READ_BTN == false) { // 确认仍然是按下状态
                keyIsPressed = true;
                clickTime = HAL_GetTick();
                longPressIsStart = false; // 为新按下重置标志
            }
        }
    }

    // 阶段2：已按下状态的处理
    if (keyIsPressed == true) {
        if (READ_BTN == false) { // 按键仍然按着
            // 检查是否达到长按时间且尚未触发长按
            if ((HAL_GetTick() - clickTime > SHORT_PRESS_TIME) && !longPressIsStart) {
                long_click_handle(); // 触发长按开始事件
                longPressIsStart = true;
            }
        }
        else { // 按键已经释放 (READ_BTN == true)
            // 根据按下总时间决定触发哪种释放事件
            if (HAL_GetTick() - clickTime > SHORT_PRESS_TIME) {
                long_click_free_handle(); // 长按后释放
                longPressIsStart = false;
            } else {
                short_click_handle(); // 短按释放
            }
            keyIsPressed = false;
        }
    }
}

void short_click_handle() {
    printf("short_click_handle\n");
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 高电平灭
}

void long_click_handle() {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    printf("long_click_handle\n");
}

void long_click_free_handle() {
    printf("click_free_handle\n");
}