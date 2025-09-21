//
// Created by 温厂长 on 2025/9/14.
//

/*
 * x打印机的原理是逐行打印，具体为打印一行后，
 * 电机拖动纸张前移一行，然后接着打印第二行，再前移，重复直至内容打印完成。
 */
#include "dr_motor.h"

uint8_t motor_pos = 0;

osTimerId myMotorTimerHandle;   // 电机软件定时器

/*
 * 半步 拍驱动方式（半步模式）
 * 这是控制步进电机旋转的核心。表中的每一行代表一个脉冲步骤，
 * 控制四个引脚（A相、A相-、B相、B相-）的输出状态。IN1 IN2 IN3 IN4
 */
uint8_t motor_table[8][4] =
{
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {1, 0, 1, 0},
    {1, 0, 0, 0},
    {1, 0, 0, 1},
    {0, 0, 0, 1},
    {0, 1, 0, 1},
    {0, 1, 0, 0}
};


//  RTOS 软件定时器 用的回调函数，每次定时器触发都会执行一次。定时器周期决定了电机的速度
void read_motor_timer_callbackfun(void const *argument)
{
    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port, MOTOR_IN1_Pin, motor_table[motor_pos][0]);
    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, motor_table[motor_pos][1]);
    HAL_GPIO_WritePin(MOTOR_IN3_GPIO_Port, MOTOR_IN3_Pin, motor_table[motor_pos][2]);
    HAL_GPIO_WritePin(MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, motor_table[motor_pos][3]);
    motor_pos++;
    if (motor_pos >= 8)
    {
        motor_pos = 0;
    }
}

/**
 * @brief  创建一个 周期性 RTOS 软件定时器，周期是 2 tick = =2ms
 */
void motor_start()
{
    if (myMotorTimerHandle == NULL)
    {
        osTimerDef(myMotorTimer, read_motor_timer_callbackfun);
        myMotorTimerHandle = osTimerCreate(osTimer(myMotorTimer), osTimerPeriodic, NULL);
        osTimerStart(myMotorTimerHandle, 2);
    }
}

// 停止
void motor_stop() {

    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port, MOTOR_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN3_GPIO_Port, MOTOR_IN3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, GPIO_PIN_RESET);

    // 删除定时器
    if(myMotorTimerHandle != NULL)
        osTimerStop(myMotorTimerHandle);
}

/**
 * @brief 手动执行一个步进，非阻塞只执行一步适合调试或点动控制
 */
void motor_run() {

    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port, MOTOR_IN1_Pin, motor_table[motor_pos][0]);
    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, motor_table[motor_pos][1]);
    HAL_GPIO_WritePin(MOTOR_IN3_GPIO_Port, MOTOR_IN3_Pin, motor_table[motor_pos][2]);
    HAL_GPIO_WritePin(MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, motor_table[motor_pos][3]);
    motor_pos++;
    if (motor_pos >= 8) {
        motor_pos = 0;
    }

}

/**
 * @brief  运行指定步数
 * @param steps    步数
 */
void motor_run_step(uint32_t steps)
{
    while (steps)
    {
        motor_run();
        HAL_Delay(4);
        us_delay(MOTOR_WATI_TIME);  // 4000 us
        steps--;
    }
}

