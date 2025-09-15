#include "dr_adc.h"
#include <math.h>
#include "adc.h"
#include "stm32f1xx_hal_adc.h"

static uint32_t adc_alg_handle(const uint32_t *adc, int size);
static float temp_calculate(float Rt);

#define ADC_READ_TIME 10 // 读取次数
uint32_t ADC_Value[2]; // 储存读取的电压、温度值

/**
 * @brief ADC初始化
 */
void adc_init() {
    HAL_ADCEx_Calibration_Start(&hadc1);    // 进行一次校准
}

/**
 * @brief 测量ADC的值
 * @return 返回测量电压值
 */
float get_adc_volts(void) {
    uint32_t adc1[ADC_READ_TIME], adc2[ADC_READ_TIME];

    for (int sample_ptr = 0; sample_ptr < ADC_READ_TIME; sample_ptr++) {
        // 第一次Start：转换通道1（电压）
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc1[sample_ptr] = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);

        // 第二次Start：转换通道2（温度）
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc2[sample_ptr] = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }

    ADC_Value[0] = adc_alg_handle(adc1, ADC_READ_TIME);
    ADC_Value[1] = adc_alg_handle(adc2, ADC_READ_TIME);

    return (float)ADC_Value[0] * 3.3f / 4096;
}

/**
 * @brief 获取温度值
 */
float get_adc_temperature() {
    // 使用之前已经处理好的ADC值
    float vol = (float)ADC_Value[1] * 3.3f / 4096;
    float Rt = (vol * 10000) / (3.3f - vol);
    return temp_calculate(Rt);
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