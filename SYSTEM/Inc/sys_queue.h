//
// Created by 温厂长 on 2025/9/17.
//

#ifndef MINIPRINTER_SYS_QUEUE_H
#define MINIPRINTER_SYS_QUEUE_H
#include "common.h"

//一行最大byte
#define MAX_ONELINE_BYTE 48 // 打印机最大
//最大行数
#define MAX_LINE 200

typedef struct{
    uint8_t buffer[MAX_ONELINE_BYTE];
}ble_rx_buffer_t;

typedef struct{
    ble_rx_buffer_t printer_buffer[MAX_LINE];
    uint32_t r_index;
    uint32_t w_index;
    uint32_t left_line;
}ble_rx_t;


void sys_queue_init(void);
void write_to_print_buffer(uint8_t *pdata, size_t length);
uint8_t *read_to_printer(void);
void clean_print_buffer(void);

uint32_t get_ble_rx_left_line(void);
#endif //MINIPRINTER_SYS_QUEUE_H