//
// Created by 温厂长 on 2025/9/17.
//
#include "sys_queue.h"
ble_rx_t g_ble_rx;

SemaphoreHandle_t xHandler = NULL;  // 用于引用一个信号量对象

/**
 * @brief 清空接收缓存
 *
 */
void clean_printbuffer()
{
    g_ble_rx.w_index = 0;    // 读指针
    g_ble_rx.r_index = 0;   // 写指针
    g_ble_rx.left_line = 0; // 队列中剩余的行数
}

void sys_queue_init(void) {
    clean_printbuffer();
    // 互斥量的作用是保护共享资源，
    // 让 多个任务或 ISR 访问同一资源时不会发生冲突
    xHandler = xSemaphoreCreateMutex();
}

/**
 * @brief 写入一行数据
 *
 * @param pdata
 * @param length
 */
void write_to_printbuffer(uint8_t *pdata, size_t length)
{
    // 标记在释放信号量后是否有比当前任务优先级更高的任务需要立即执行
    static BaseType_t xHigherPriorityTaskWoken;
    if (length == 0)
        return;
    if (g_ble_rx.left_line >= MAX_LINE) // 队列已满，丢弃数据
        return;
    if (length > MAX_ONELINE_BYTE)  //如果一行数据长度超过允许最大长度，截断
        length = MAX_ONELINE_BYTE;
    // 查看是否可以获得信号量，如果信号量不可用，则用10个时钟滴答来查看信号量是否可用
    // 在 ISR 中尝试获取互斥量，保护 g_ble_rx 共享资源， 不会阻塞任务
    if (xSemaphoreTakeFromISR(xHandler, &xHigherPriorityTaskWoken) == pdPASS)   //
    {
        // 将 pdata 的数据复制到环形队列当前写入位置
        memcpy(&g_ble_rx.printer_buffer[g_ble_rx.w_index], pdata, length);
        g_ble_rx.w_index++;
        g_ble_rx.left_line++;

        // 写指针回绕
        if (g_ble_rx.w_index >= MAX_LINE)
            g_ble_rx.w_index = 0;
        if (g_ble_rx.left_line >= MAX_LINE)
            g_ble_rx.left_line = MAX_LINE;
        xSemaphoreGiveFromISR(xHandler,&xHigherPriorityTaskWoken); // ISR 安全的释放信号量函数
    }
    if(xHigherPriorityTaskWoken == pdTRUE){
        // 如果有更高优先级的任务需要唤醒，则进行任务切换
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


/**
 * @brief 从全局打印队列 g_ble_rx 中读取一行数据（环形队列）
 * @return 返回指向一行数据的指针，如果队列为空或获取互斥量失败则返回 NULL
 */
uint8_t *read_to_printer()
{
    // 查看是否可以获得信号量，如果信号量不可用，则用10个时钟滴答来查看信号量是否可用
    if (xSemaphoreTake(xHandler, (portTickType)10) == pdPASS)
    {
        if (g_ble_rx.left_line > 0)
        {
            uint32_t index = 0;
            g_ble_rx.left_line--;
            index = g_ble_rx.r_index;
            g_ble_rx.r_index++;
            if (g_ble_rx.r_index >= MAX_LINE)
                g_ble_rx.r_index = 0;
            xSemaphoreGive(xHandler);
            return g_ble_rx.printer_buffer[index].buffer;
        }
        xSemaphoreGive(xHandler);
        return NULL;
    }
    return NULL;
}
