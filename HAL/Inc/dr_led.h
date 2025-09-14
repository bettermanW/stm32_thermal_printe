//
// Created by 温厂长 on 2025/9/14.
//

#ifndef MINIPRINTER_DR_LED_H
#define MINIPRINTER_DR_LED_H


#include "common.h"

/**
 * 状态灯
 * 待机   暗
 * 蓝牙连接 亮
 * 运行异常 快闪
 * 打印中  慢闪
 */
typedef  enum {
    WAIT,
    CONNECTING,
    WRONG,
    PRINTING,
}led_state;

void led_run_state(led_state state);
#endif //MINIPRINTER_DR_LED_H