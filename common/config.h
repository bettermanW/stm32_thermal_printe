//
// Created by 温厂长 on 2025/9/20.
//

#ifndef MINIPRINTER_CONFIG_H
#define MINIPRINTER_CONFIG_H
#include "common.h"

//接收完成所有数据才开始打印
#define START_PRINTER_WHEN_FINISH_RAED 0

#define PRINT_TIME 1700         //打印加热时间
#define PRINT_END_TIME 200      //冷却时间
#define MOTOR_WATI_TIME 4000    //电机一步时间
#define LAT_TIME 1              //数据锁存时间

#include "tim.h"
#define DLY_TIM_Handle (&htim1)  // Timer handle
#define hal_delay_us(nus) do { \
__HAL_TIM_SET_COUNTER(DLY_TIM_Handle, 0); \
__HAL_TIM_ENABLE(DLY_TIM_Handle); \
while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < (nus)); \
__HAL_TIM_DISABLE(DLY_TIM_Handle); \
} while(0)

#define us_delay(us) hal_delay_us(us)

#endif //MINIPRINTER_CONFIG_H