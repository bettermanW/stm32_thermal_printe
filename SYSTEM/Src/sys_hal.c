//
// Created by 温厂长 on 2025/9/21.
//
#include "sys_hal.h"

int freq = 2000;    // 设置频率2000kHz
int channel = 0;    // 通道号，取值0 ~ 15
int resolution = 8; // 分辨率，取值0~20，占空比duty最大取值为2^resolution-1
bool need_report = false;
#define EPISON 1e-7



__inline static uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);
static void read_temperature();
static void read_battery();
static void read_paper_status(void);


// 读取设备状态
void read_all_hal()
{
    read_battery();
    read_temperature();
    read_paper_status();
}

void led_run_state(const led_state_e state) {
    switch (state) {
        case LED_CONNECTED:
            led_on(); // 亮
            break;
        case LED_DISCONNECTED:
            led_off(); // 灭
            break;
        case LED_WARONG:
            led_flash(3, 200);  //快速闪烁3次，间隔200ms
            break;
        case LED_PRINT_START:   // 单次闪烁（亮-灭）
            led_flash(1, 200);
            break;
        case LED_BLE_INIT:
            led_flash(1, 10);   // 极快速闪烁一次
        default:
            break;
    }
}

/********************************纸张检测********************************/

/**
 * @brief 纸张检测
 *        检测IO如果为高则打印缺纸信息
 */
static void read_paper_status(void) {
    if (HAL_GPIO_ReadPin(PAINT_GPIO_Port, PAINT_Pin) == GPIO_PIN_SET) {
        get_device_state()->paper_state = PAPER_STATE_LACK;
    }else {
        get_device_state()->paper_state = PAPER_STATE_NORMAL;
    }
    printf("paper_state = %d\n",get_device_state()->paper_state);
}

// 中断检测纸张状态
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PAINT_Pin) // 判断是否是PA0触发的中断
    {
        need_report = true;
        get_device_state()->paper_state = PAPER_STATE_LACK;
    }
}

bool read_paper_irq_need_report_status()
{
    if (need_report)
    {
        need_report = false;
        return true;
    }
    else
        return false;
}


/**************************************ADC读取*********************************/

/*读取打印机温度*/
static void read_temperature()
{
    const float temperature = get_adc_temperature();

    // 添加合理的温度范围检查
    if(temperature >= -40.0f && temperature <= 125.0f) {  // 典型MCU温度范围
        get_device_state()->temperature = (uint8_t)temperature;  // 转换为整数
        printf("temperature = %.1fC\n", temperature);
    } else {
        get_device_state()->temperature = 0;
        printf("Temperature sensor error: %.1fC\n", temperature);
    }
}

// 电池读取增加错误处理
static void read_battery()
{
    const float voltage = get_adc_volts() * 2;  // 假设有2倍分压
    if(voltage < 0) {
        printf("ADC read error!\n");
        return;
    }

    uint32_t battery_percent = map(voltage, 3300, 4200, 0, 100);
    battery_percent = battery_percent > 100 ? 100 : battery_percent;
    battery_percent = battery_percent < 0 ? 0 : battery_percent;

    get_device_state()->battery = battery_percent;
    printf("battery = %ld%%, voltage = %.2fmV\n", battery_percent, voltage);
}



/**
 * @brief 映射函数
 * @param x 将数值 x从输入范围 [in_min, in_max]线性映射到输出范围 [out_min, out_out_max]
 * @param in_min 映射前最小
 * @param in_max 映射前最大
 * @param out_min 映射后最小
 * @param out_max 映射前最大
 * @return
 */
__inline static uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max) {
    if (in_min == in_max) {
        return (in_min + out_max) / 2;
    }
    const uint32_t dividend = out_max - out_min; // 输出范围跨度
    const uint32_t divisor = in_max - in_min;   // 输入范围跨度
    const uint32_t delta = x - in_min;  // 输入值相对于最小值的偏移

    uint64_t big_result = (uint64_t)delta * dividend;
    big_result += divisor / 2;
    big_result /= divisor;
    big_result += out_min;

    if(big_result > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)big_result;
}