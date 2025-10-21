//
// Created by 温厂长 on 2025/9/15.
//

#include "dr_printer.h"
#include "spi.h"



// 根据打印头实际打印效果修改打印时间偏移值
#define STB1_ADD_TIME 0
#define STB2_ADD_TIME 0
#define STB3_ADD_TIME 0
#define STB4_ADD_TIME 0
#define STB5_ADD_TIME 0
#define STB6_ADD_TIME 0

// 热密度
uint8_t heat_density = 64;

void set_heat_density(const uint8_t density)
{
    printf("打印密度设置 %d\n", density);
    heat_density = density;
}

/**
 * @brief 失能所有通道 6个脉冲设为低电平
 *
 */
static void set_stb_idle()
{
    HAL_GPIO_WritePin(STB1_GPIO_Port, STB1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STB2_GPIO_Port, STB2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STB3_GPIO_Port, STB3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STB4_GPIO_Port, STB4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STB5_GPIO_Port, STB5_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STB6_GPIO_Port, STB6_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  打印前初始化
 */
static void init_printing() {

    // 开启打印超时监听 定时20ms
    open_printer_timeout_timer();
    //失能6个通道
    set_stb_idle();
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);    // 锁存器设为高电平
    // 开启电源
    HAL_GPIO_WritePin(VH_EN_GPIO_Port, VH_EN_Pin, GPIO_PIN_SET);
}

static  void stop_printing() {

    close_printer_timeout_timer();
    HAL_GPIO_WritePin(VH_EN_GPIO_Port, VH_EN_Pin, GPIO_PIN_RESET);
    set_stb_idle();
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);

}


float addTime[6] = {0};
// 点数-增加时间系数
#define kAddTime 0.001


static void clearAddTime()
{
    memset(addTime, 0, sizeof(addTime));
}


/**
 * @brief  发送一行数据
 * @param data 数据
 */
static void send_one_line_data(const uint8_t *data) {

    float tmpAddTime = 0;
    clearAddTime();

    for (uint8_t i = 0; i < 6; ++i)
    {
        for (uint8_t j = 0; j < 8; ++j)
        {
            addTime[i] += data[i * 8 + j];
        }
        tmpAddTime = addTime[i] * addTime[i];
        addTime[i] = kAddTime * tmpAddTime;
    }

    HAL_SPI_Transmit(&hspi1, data, sizeof(data), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_RESET);
    us_delay(LAT_TIME);
}

/**
 * @brief 通道打印运行
 * @param now_stb_num 选用哪个通道
 */
static void run_stb(uint8_t now_stb_num) {

    switch (now_stb_num) {
        case 0:
            HAL_GPIO_WritePin(STB1_GPIO_Port, STB1_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[0] + STB1_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB1_GPIO_Port, STB1_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        case 1:
            HAL_GPIO_WritePin(STB2_GPIO_Port, STB2_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[1] + STB2_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB2_GPIO_Port, STB2_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        case 2:
            HAL_GPIO_WritePin(STB3_GPIO_Port, STB3_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[2] + STB3_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB3_GPIO_Port, STB3_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        case 3:
            HAL_GPIO_WritePin(STB4_GPIO_Port, STB4_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[3] + STB4_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB4_GPIO_Port, STB4_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        case 4:
            HAL_GPIO_WritePin(STB5_GPIO_Port, STB5_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[4] + STB5_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB5_GPIO_Port, STB5_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        case 5:
            HAL_GPIO_WritePin(STB6_GPIO_Port, STB6_Pin, GPIO_PIN_SET);
            us_delay((PRINT_TIME + addTime[5] + STB6_ADD_TIME) * ((double)heat_density / 100));
            HAL_GPIO_WritePin(STB6_GPIO_Port, STB6_Pin, GPIO_PIN_RESET);
            us_delay(PRINT_END_TIME);
            break;
        default:
            break;

    }
}

/**
 * @brief 移动电机&开始打印
 *
 * @param need_stop
 * @param stb_num
 */
bool move_and_start_std(bool need_stop, uint8_t stb_num)
{
    if (need_stop == true)
    {
        printf("打印停止\n");
        motor_stop();
        stop_printing();
        return true;
    }
    // 4step一行
    motor_run();
    if (stb_num == ALL_STB_NUM)
    {
        // 所有通道打印
        for (uint8_t index = 0; index < 6; index++)
        {
            run_stb(index);
            // 把电机运行信号插入通道加热中，减少打印卡顿和耗时
            if (index == 1 || index == 3 || index == 5)
            {
                motor_run();
            }
        }
        // motor_run_step(3);
    }
    else
    {
        // 单通道打印
        run_stb(stb_num);
        motor_run_step(3);
    }
    return false;
}


/**
 * @brief 打印错误检查
 *
 * @param need_report 是否需BLE上报
 * @return true 打印出错
 * @return false 打印正常
 */
bool printing_error_check(bool need_report)
{
    if (get_printer_timeout_status())
    {
        printf("打印超时\n");
        return true;
    }
    if (get_device_state()->paper_state == PAPER_STATE_LACK)
    {
        if(need_report){
            // 停止打印
            clean_print_buffer();
            ble_report();
        }
        // 停止打印
        printf("缺纸\n");
        led_run_state(LED_WARONG);
        return true;
    }
    if (get_device_state()->temperature > 65)
    {
        if(need_report){
            // 停止打印
            clean_print_buffer();
            ble_report();
        }
        // 停止打印
        printf("温度异常\n");
        led_run_state(LED_WARONG);
        return true;
    }
    return false;
}


/**
 * @brief 数组打印
 *
 * @param data
 * @param len 数据长度必须是整行 48*n
 */
void start_printing(const uint8_t *data, const uint32_t len)
{
    uint32_t offset = 0;
    const uint8_t *ptr = data;
    bool need_stop = false;
    init_printing();
    while (1)
    {
        if (len > offset)
        {
            // 发送一行数据 48byte*8=384bit
            send_one_line_data(ptr);
            offset += TPH_DI_LEN;
            ptr += TPH_DI_LEN;
        }
        else
            need_stop = true;
        if (move_and_start_std(need_stop, ALL_STB_NUM))
            break;
        if(printing_error_check(false))
            break;
    }
    motor_run_step(40);
    motor_stop();
    printf("打印完成\n");
}

/**
 * @brief 可变队列打印
 *
 */
void start_printing_by_queue_buf()
{
    uint8_t *pdata = NULL;
		uint32_t printer_count = 0;
    init_printing();
    while (1)
    {
        if (get_ble_rx_left_line() > 0)
        {
            // printf("printing...\n");
            pdata = read_to_printer();
            if (pdata != NULL)
            {
								printer_count ++;
                send_one_line_data(pdata);
                if (move_and_start_std(false, ALL_STB_NUM))
                    break;
            }
        }
        else
        {
            if (move_and_start_std(true, ALL_STB_NUM))
                break;
        }
        if (get_printer_timeout_status())
            break;
        if(printing_error_check(true))
            break;
    }
    motor_run_step(140);
    motor_stop();
    clean_ble_pack_count();
    printf("printer finish !!! read=%ld printer:%ld\n",get_ble_pack_count(),printer_count);
}

/**
 * @brief 单通道数组打印
 *
 * @param stb_num
 * @param data
 * @param len
 */
void start_printing_by_one_stb(uint8_t stb_num, uint8_t *data, uint32_t len)
{
    uint32_t offset = 0;
    uint8_t *ptr = data;
    bool need_stop = false;
    init_printing();
    while (1)
    {
        printf("printer %ld\n", offset);
        if (len > offset)
        {
            // 发送一行数据 48byte*8=384bit
            send_one_line_data(ptr);
            offset += TPH_DI_LEN;
            ptr += TPH_DI_LEN;
        }
        else
            need_stop = true;
        if (move_and_start_std(need_stop, stb_num))
            break;
        if (get_printer_timeout_status())
            break;
        if(printing_error_check(false))
            break;
    }
		printf("printer end\n");
    motor_run_step(40);
    motor_stop();
		printf("printer end2\n");
}

static void setDebugData(uint8_t *print_data)
{
    for (uint32_t cleardata = 0; cleardata < 48 * 5; ++cleardata)
    {
        print_data[cleardata] = 0x55;
    }
}

void testSTB()
{
    uint8_t print_data[48 * 6];
    printf("开始打印打印头选通引脚测试\n顺序: 1  2  3  4  5  6");
    uint32_t print_len = 48 * 5;
    setDebugData(print_data);
    start_printing_by_one_stb(0, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_one_stb(1, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_one_stb(2, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_one_stb(3, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_one_stb(4, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_one_stb(5, print_data, print_len);
    printf("测试完成");
}

