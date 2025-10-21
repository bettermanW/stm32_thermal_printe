#include "dr_adc.h"
#include <math.h>
#include "adc.h"
#include "stm32f1xx_hal_adc.h"

static uint32_t adc_alg_handle(const uint32_t *adc, int size);
static float temp_calculate(float Rt);

uint16_t ADC_Value[3]; // 储存读取的电压、温度值

/**
 * @brief ADC初始化
 */
void adc_init() {
    HAL_ADCEx_Calibration_Start(& hadc1); // 校准
    HAL_ADC_Start_DMA(&hadc1, (uint32_t * )ADC_Value, 3);
}

/**
 * @brief 测量ADC的值
 * @return 返回测量电压值
 */
float get_adc_volts(void) {
    // return ADC_Value[0];
    return (float)ADC_Value[0] * 3.3f / 4096.0f;
}

/**
 * @brief 获取温度值
 */
float get_adc_temperature() {
    // 使用之前已经处理好的ADC值
    float vol = (float)ADC_Value[1] * 3.3f / 4096;
    float Rt = (vol * 10000) / (3.3f - vol);
    // return ADC_Value[1];
    return temp_calculate(Rt);
}

float get_adc_ver_fint(void) {
    // return ADC_Value[2] *  3.3f / 4096;
    return ADC_Value[2];
}

/**
 * @brief 公式法求温度
 */
static float temp_calculate(const float Rt) {
    const float Rp = 10000; // 注意：根据实际热敏电阻修改（通常为10k）
    const float T2 = 25 + 273.15f;
    const float B = 3950;
    const float Ka = 273.15f;

    return 1 / (logf(Rt / Rp) / B + 1 / T2) - Ka + 0.5f;
}

/**
 * @brief 数据处理：去掉最小最大值后取平均
 */
static uint32_t adc_alg_handle(const uint32_t *adc, int size) {
    uint32_t sum = 0;
    uint32_t max_adc = 0;
    uint32_t min_adc = UINT32_MAX;

    for (int i = 0; i < size; i++) {
        if (adc[i] > max_adc) {
            max_adc = adc[i];
        }
        if (adc[i] < min_adc) {
            min_adc = adc[i];
        }
        sum += adc[i];
    }

    sum -= (min_adc + max_adc);
    return sum / (size - 2);
}