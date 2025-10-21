//
// Created by 温厂长 on 2025/9/14.
//

#ifndef MINIPRINTER_DR_MOTOR_H
#define MINIPRINTER_DR_MOTOR_H


//V3 电机引脚
#include "common.h"
void motor_start();

void motor_run(void);
void motor_run_step(uint32_t steps);
void motor_stop(void);
#endif //MINIPRINTER_DR_MOTOR_H