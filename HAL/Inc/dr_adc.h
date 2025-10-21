//
// Created by 温厂长 on 2025/9/12.
//

#ifndef MINIPRINTER_DR_ADC_H
#define MINIPRINTER_DR_ADC_H
#include "common.h"
void adc_init(void);
float get_adc_volts(void);

float get_adc_temperature(void);

float get_adc_volts_filtered(void);
float get_adc_ver_fint(void);
void em_adc_test();
#endif //MINIPRINTER_DR_ADC_H