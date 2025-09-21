//
// Created by 温厂长 on 2025/9/12.
//

#ifndef MINIPRINTER_DR_BLE_H
#define MINIPRINTER_DR_BLE_H
#include "common.h"

void init_ble(void);
void uart_cmd_handle(uint8_t data);
void ble_report(void);

void clean_ble_pack_count();
uint32_t get_ble_pack_count();
bool get_ble_connect();
#endif //MINIPRINTER_DR_BLE_H