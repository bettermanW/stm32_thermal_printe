//
// Created by 温厂长 on 2025/9/21.
//
#include "task.h"

bool printer_test = false;
bool is_long_click_start = false;

void key_click_handle()
{
    printf("Button 单击!\n");
    printer_test = true;
    // read_all_hal();
}

void key_long_click_handle()
{
    if(is_long_click_start == true)
        return;
    is_long_click_start = true;
    printf("Button 长按!\n");
    device_state_t *pdevice = get_device_state();
    bool need_beep = false;
    // 不缺纸且不在打印中才执行
    if (pdevice->paper_state == PAPER_STATE_NORMAL)
    {
        if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
            pdevice->printer_state == PRINTER_STATUS_INIT)
        {
            printf("开始走纸\n");
            motor_start();
        }
        else
        {
            need_beep = true;
        }
    }
    else
        need_beep = true;
    if (need_beep)
    {
        led_run_state(LED_WARONG);
    }
}

void key_long_click_free_handle()
{
    is_long_click_start = false;
    printf("停止走纸\n");
    motor_stop();
}


/***/
void task_button(void *pvParameters)
{
    // int count = 0;
    printf("task_button init\n");
    for (;;) {
        key_check_run();   // 处理按键状态机
        vTaskDelay(20);     // 每 20 tick 执行一次
        // count++;
        // if (count >= 250) {
        //     count = 0;
        //     printf("task_button run\n");
        // }
    }
}



/********************************/
// 周期性检查是否需要上报状态
void run_report() {

    if (get_state_timeout()) { // 如果定时到达 10s
        clean_state_timeout();
        read_all_hal();   // 读取电池、电机、打印机等状态
        if (get_ble_connect()) { // 蓝牙是否连接
            printf("report device status:report time up\n");
            ble_report(); // 蓝牙上报状态
        }
    }

    // 是否缺纸中断触发
    if (read_paper_irq_need_report_status()) {
        printf("report device status : paper irq\n");
        ble_report();
    }
}

/**
 * @brief 处理打印相关事件
 *
 */
void run_printer()
{
    device_state_t *pdevice = get_device_state();
#ifdef START_PRINTER_WHEN_FINISH_RAED // 接收完在打印
    if (pdevice->read_ble_finish == true)
    {
        if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
            pdevice->printer_state == PRINTER_STATUS_INIT)
        {
            pdevice->read_ble_finish = false;
            pdevice->printer_state = PRINTER_STATUS_START;
            ble_report();
            printf("report device status : printing start %ld\n",get_ble_rx_left_line());
            led_run_state(LED_PRINT_START);
        }
    }
#else
    // 接收大于100条时，才触发开始打印
    if (get_ble_rx_leftline()> 200)
    {
        if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
                pdevice->printer_state == PRINTER_STATUS_INIT)
        {
            pdevice->printer_state = PRINTER_STATUS_START;
            ble_report();
            printf("report device status : printing start\n");
            led_run_state(LED_PRINT_START);
        }
    }
#endif
    // 开始打印
    if (pdevice->printer_state == PRINTER_STATUS_START)
    {
        // 正常打印
        start_printing_by_queue_buf();
        pdevice->printer_state = PRINTER_STATUS_FINISH;
    }
}


void task_printer(void *pvParameters)
{
    // int count = 0;
    init_ble();
    printf("task_printer init\n");
    for (;;) {
        ble_status_data_clean();
        run_printer();
        vTaskDelay(1);   // 1 tick 周期
        if (printer_test) {
            printer_test = false;
            // test_black_line();
            testSTB();   // 执行测试打印
        }
        // count++;
        // if (count >= 5000) {
        //     count = 0;
        //     printf("task_printer run\n");
        // }
    }
}


/**
 * @brief 每 100 tick 执行一次状态上报， 每50次打印debug
 * @param pvParameters
 */
void task_report(void *pvParameters)
{
    // int count = 0;
    printf("task_report init\n");
    for (;;) {
        run_report();
        vTaskDelay(100);   // 延时100 tick (~100ms)
        // count++;
        // if (count >= 50) {
        //     count = 0;
        //     printf("task_report run\n");
        // }
    }
}


void init_task(void) {

    device_state_init();
    init_timer();
    sys_queue_init();
    adc_init();
    // init_ble();



    xTaskCreate(
        task_button,  // 任务函数
        "TaskButton", // 任务名
        128,          // 任务栈
        NULL,         // 任务参数
        0,            // 任务优先级, with 3 (configMAX_PRIORITIES - 1) 是最高的，0是最低的.
        NULL          // 任务句柄
    );

    xTaskCreate(
       task_report,  // 任务函数
       "TaskReport", // 任务名
       256,          // 任务栈
       NULL,         // 任务参数
       1,            // 任务优先级, with 3 (configMAX_PRIORITIES - 1) 是最高的，0是最低的.
       NULL          // 任务句柄
   );


    xTaskCreate(
        task_printer,  // 任务函数
        "TaskPrinter", // 任务名
        256,           // 任务栈
        NULL,          // 任务参数
        2,             // 任务优先级, with 3 (configMAX_PRIORITIES - 1) 是最高的，0是最低的.
        NULL           // 任务句柄
    );
}