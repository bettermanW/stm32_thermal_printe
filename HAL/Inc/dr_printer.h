//
// Created by 温厂长 on 2025/9/15.
//

#ifndef MINIPRINTER_DR_PRINTER_H
#define MINIPRINTER_DR_PRINTER_H

#include "common.h"

//每行总点数
#define TPH_DOTS_PER_LINE 384
//每行字节长度 384/8 (有效打印宽度)
#define TPH_DI_LEN 48 // 一行384个01， 48byte
//所有通道打印
#define ALL_STB_NUM 0xFF


void set_heat_density(uint8_t density);

/**
 * @brief 数组打印
 *
 * @param data
 * @param len 数据长度必须是整行 48*n
 */
void start_printing(const uint8_t *data, uint32_t len);

/**
 * @brief 可变队列打印
 *
 */
void start_printing_by_queue_buf(void);

void start_printing_by_one_stb(uint8_t stb_num, uint8_t *data, uint32_t len);


void testSTB();

void test_black_line(void);
#endif //MINIPRINTER_DR_PRINTER_H