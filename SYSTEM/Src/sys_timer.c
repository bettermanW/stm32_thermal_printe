//
// Created by 温厂长 on 2025/9/16.
//
#include "sys_timer.h"

osTimerId myStateTimerHandle;   //状态定时器句柄
osTimerId myTimeoutTimerHandle; // 打印超时定时器句柄

bool read_state_timeout = false;    // 状态轮询定时器超时标志
bool printer_timeout = false;   // 打印超时标志


/* read_state_timer_callbackfun function */
void read_state_timer_callbackfun(void const * argument)
{
    /* USER CODE BEGIN read_state_timer_callbackfun */
    // 每次触发时打印提示
    printf("read_state now...\r\n");
    read_state_timeout = true;
    /* USER CODE END read_state_timer_callbackfun */
}

/*打印超时回调函数*/
void read_timeout_timer_callbackfun(void const * argument)
{
    printf("触发打印超时错误...\r\n");
    printer_timeout = true;
}

void init_timer(){
    // 定义一个定时器，名字叫 myStateTimer，回调函数是 read_state_timer_callbackfun
    osTimerDef(myStateTimer, read_state_timer_callbackfun);

    // 创建定时器，模式是周期性 (osTimerPeriodic)，参数为 NULL
    myStateTimerHandle = osTimerCreate(osTimer(myStateTimer), osTimerPeriodic, NULL);

    // 启动定时器，周期 10000 个 tick（不是毫秒，要看 osKernelSysTickFrequency）
    osTimerStart(myStateTimerHandle, 10000);
}

void open_printer_timeout_timer(){
    printer_timeout = false;
    osTimerDef(myTimeoutTimer, read_timeout_timer_callbackfun);
    myTimeoutTimerHandle = osTimerCreate(osTimer(myTimeoutTimer), osTimerOnce, NULL);
    osTimerStart(myTimeoutTimerHandle,20000);   // 20ms
}

/*查询定时器是否超时*/
bool get_state_timeout(){
    return read_state_timeout;
}

/*清除标志，通常在处理完状态超时后调用*/
void clean_state_timeout(){
    read_state_timeout = false;
}

void close_printer_timeout_timer(){
    osTimerDelete(myTimeoutTimerHandle);
}