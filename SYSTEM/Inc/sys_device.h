//
// Created by 温厂长 on 2025/9/14.
//

#ifndef MINIPRINTER_SYS_DEVICE_H
#define MINIPRINTER_SYS_DEVICE_H
#include "common.h"


typedef enum{
    PRINTER_STATUS_INIT = 0,
    PRINTER_STATUS_START,
    PRINTER_STATUS_WORKING,
    PRINTER_STATUS_FINISH,
}printer_state_e;

typedef enum{
    PAPER_STATE_LACK,
    PAPER_STATE_NORMAL
}paper_state_e; // 是否缺纸状态

typedef struct {
    uint8_t battery;
    uint8_t temperature;
    paper_state_e paper_state;
    bool read_ble_finish;
    printer_state_e printer_state;
}device_state_t;    // 设备状态定义

void device_state_init(void);
void set_device_paper_staus(paper_state_e status);
device_state_t * get_device_state(void);

void set_read_ble_finish(bool finish);


#endif //MINIPRINTER_SYS_DEVICE_H