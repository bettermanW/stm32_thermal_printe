//
// Created by 温厂长 on 2025/9/14.
//

#include "sys_device.h"

#include <stdio.h>

#include "main.h"
#include "stm32f1xx_hal_gpio.h"

device_state_t g_device_state;


/**
 * @brief  配置初始化
 */
void device_state_init() {
    memset(&g_device_state,0,sizeof(device_state_t));
    g_device_state.printer_state = PRINTER_STATUS_INIT;
    g_device_state.battery = 10;
    g_device_state.temperature = 30;
    g_device_state.paper_state = PAPER_STATE_NORMAL;
    g_device_state.read_ble_finish = false;
}

/**
 * @brief 设置打印机的地址
 * @param status 打印机的状态
 */
void set_device_paper_staus(const paper_state_e status) {
    g_device_state.paper_state = status;
}

/**
 * @brief 获取设备状态
 * @return 状态 用地址快
 */
device_state_t * get_device_state(void) {
    return &g_device_state;
}



/**
 * @brief 设置蓝牙连接完成
 * @param finish
 */
void set_read_ble_finish(bool finish){
    g_device_state.read_ble_finish = finish;
}








