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
#define PRINT_TEMP_LIMIT    65
#define LAT_PULSE_US        10
#define COOL_DOWN_DELAY_MS  100
#define SAFE_STOP_DELAY_MS  50

float addTime[6] = {0};
// 点数-增加时间系数
#define kAddTime 0.001


/**
 * @brief  设置打印密度
 * @param density 打印密度
 */
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
    // 锁存器设为高电平
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);
    // 开启电源
    HAL_GPIO_WritePin(VH_EN_GPIO_Port, VH_EN_Pin, GPIO_PIN_SET);
}

static  void stop_printing(const char *reason) {

    printf("[STOP] 打印结束：%s\n", reason);
    vTaskDelay(pdMS_TO_TICKS(COOL_DOWN_DELAY_MS)); // 让热头冷却
    close_printer_timeout_timer();
    HAL_GPIO_WritePin(VH_EN_GPIO_Port, VH_EN_Pin, GPIO_PIN_RESET);
    set_stb_idle();
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);
    motor_stop();
    vTaskDelay(pdMS_TO_TICKS(SAFE_STOP_DELAY_MS));

}

static inline uint8_t bitcount(uint8_t v)
{
    uint8_t c = 0;
    for (int b = 0; b < 8; b++)
        if (v & (1 << b)) c++;
    return c;
}





static void send_one_line_data(uint8_t *data)
{
    float tmpAddTime = 0;
    memset(addTime, 0, sizeof(addTime));

    // 计算每个 STB 通道的加热时间补偿
    for (uint8_t i = 0; i < 6; ++i)
    {
        for (uint8_t j = 0; j < 8; ++j)
            addTime[i] += bitcount(data[i * 8 + j]);   // ✅ 修正点数统计
        tmpAddTime = addTime[i] * addTime[i];
        addTime[i] = kAddTime * tmpAddTime;
    }

    // 终于找到错误了，这里不能用sizeof(data) = 4; 应该是48
    for (int i = 0; i < 6; i++) printf("%02X ", data[i]);
    printf("\n");
    HAL_SPI_Transmit(&hspi1, data, TPH_DI_LEN, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_RESET);
    us_delay(LAT_TIME);
    HAL_GPIO_WritePin(LAT_GPIO_Port, LAT_Pin, GPIO_PIN_SET);
}


/**
 * @brief 通道打印运行
 * @param stb_index 选用哪个通道
 */
static void run_stb(uint8_t stb_index) {

    GPIO_TypeDef *ports[6] = {
        STB1_GPIO_Port, STB2_GPIO_Port, STB3_GPIO_Port,
        STB4_GPIO_Port, STB5_GPIO_Port, STB6_GPIO_Port
    };
    const uint16_t pins[6] = {
        STB1_Pin, STB2_Pin, STB3_Pin,
        STB4_Pin, STB5_Pin, STB6_Pin
    };

    float print_time = (PRINT_TIME + addTime[stb_index]) *
                       ((float)heat_density / 100.0f);

    HAL_GPIO_WritePin(ports[stb_index], pins[stb_index], GPIO_PIN_SET);
    us_delay(print_time);
    HAL_GPIO_WritePin(ports[stb_index], pins[stb_index], GPIO_PIN_RESET);
    us_delay(PRINT_END_TIME);
}

/**
 * @brief 移动电机&开始打印
 *
 * @param need_stop
 * @param stb_num
 */
bool move_and_start_std(bool need_stop, uint8_t stb_num)
{
    if (need_stop) return true;

    motor_run();

    if (stb_num == ALL_STB_NUM)
    {
        for (uint8_t i = 0; i < 6; i++)
        {
            run_stb(i);
            if (i % 2) motor_run();
        }
    }
    else
    {
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
    device_state_t *dev = get_device_state();

    if (get_printer_timeout_status()) {
        printf("[ERR] 打印超时\n");
        return true;
    }

    if (dev->paper_state == PAPER_STATE_LACK) {
        printf("[ERR] 缺纸\n");
        if (need_report) ble_report();
        led_run_state(LED_WARONG);
        return true;
    }

    if (dev->temperature > PRINT_TEMP_LIMIT) {
        printf("[ERR] 温度过高\n");
        if (need_report) ble_report();
        led_run_state(LED_WARONG);
        return true;
    }

    return false;
}

/**
 * @brief 可变队列打印
 *
 */
void start_printing_by_queue_buf(void)
{
    uint8_t *pdata = NULL;
    uint32_t printer_count = 0;
    const TickType_t delay_per_line = pdMS_TO_TICKS(1);

    printf("[PRINT] 开始打印\n");
    init_printing();

    while (1)
    {
        if (get_ble_rx_left_line() > 0)
        {
            pdata = read_to_printer();
            if (pdata)
            {
                printer_count++;
                send_one_line_data(pdata);
                move_and_start_std(false, ALL_STB_NUM);
            }
        }
        else
        {
            stop_printing("正常结束");
            break;
        }

        if (get_printer_timeout_status()) {
            stop_printing("超时");
            break;
        }

        if (printing_error_check(true)) {
            stop_printing("错误终止");
            break;
        }

        vTaskDelay(delay_per_line);  // 给 FreeRTOS 调度时间
    }

    motor_run_step(140);
    motor_stop();
    clean_ble_pack_count();
    printf("[PRINT] 完成: 打印行=%lu\n", printer_count);
}


/****************************测试**************************/


void test_black_line(void)
{
    uint8_t line[48];
    memset(line, 0xFF, sizeof(line));  // 全黑点
    init_printing();
    for (int i = 0; i < 50; i++) {
        send_one_line_data(line);
        move_and_start_std(false, ALL_STB_NUM);
    }
    stop_printing("reson\n");
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

