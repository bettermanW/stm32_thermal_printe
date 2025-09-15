//
// Created by 温厂长 on 2025/9/14.
//

#include "sys_device.h"

#include <stdio.h>

#include "main.h"
#include "stm32f1xx_hal_gpio.h"

device_state_t g_device_state;

__inline static uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);

/**
 * @brief  配置初始化
 */
void device_state_init() {
    g_device_state.battery = 100;
    g_device_state.temperature = 30;
    g_device_state.paper_state = PAPER_STATE_NORMAL;
}

/**
 * @brief 设置打印机的地址
 * @param status 打印机的状态
 */
void set_device_paper_staus(const paper_state_e status) {
    g_device_state.paper_state = status;
}

/**
 * @brief 获取设备状态
 * @return 状态 用地址快
 */
device_state_t * get_device_state(void) {
    return &g_device_state;
}

/**
 * @brief 纸张检测
 *        检测IO如果为高则打印缺纸信息
 */
void read_paper_status(void) {
    if (HAL_GPIO_ReadPin(PAINT_GPIO_Port, PAINT_Pin) == GPIO_PIN_SET) {
        get_device_state()->paper_state = PAPER_STATE_LACK;
    }else {
        get_device_state()->paper_state = PAPER_STATE_NORMAL;
    }
    printf("paper_state = %d\n",get_device_state()->paper_state);
}

/*读取打印机温度*/
void read_temperature()
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
void read_battery()
{
    float voltage = get_adc_volts() * 2;  // 假设有2倍分压
    if(voltage < 0) {
        printf("ADC read error!\n");
        return;
    }

    uint32_t battery_percent = map((long)voltage, 3300, 4200, 0, 100);
    battery_percent = battery_percent > 100 ? 100 : battery_percent;
    battery_percent = battery_percent < 0 ? 0 : battery_percent;

    get_device_state()->battery = battery_percent;
    printf("battery = %d%%, voltage = %.2fmV\n", battery_percent, voltage);
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
