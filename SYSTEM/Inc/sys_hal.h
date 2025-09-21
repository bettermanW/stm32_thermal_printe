//
// Created by 温厂长 on 2025/9/21.
//

#ifndef MINIPRINTER_SYS_HAL_H
#define MINIPRINTER_SYS_HAL_H
#include "common.h"

/**
 * 状态灯
 * 蓝牙已连接   暗
 * 蓝牙未连接 亮
 * 运行异常 快闪
 * 打印中  慢闪
 * 蓝牙初始化配置
 */
typedef  enum {
    LED_CONNECTED,
    LED_DISCONNECTED,
    LED_WARONG,
    LED_PRINT_START,
    LED_BLE_INIT
}led_state_e;

void read_all_hal(void);
void led_run_state(led_state_e state);
bool read_paper_irq_need_report_status(void);
#endif //MINIPRINTER_SYS_HAL_H