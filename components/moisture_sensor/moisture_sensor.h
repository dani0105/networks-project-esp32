#ifndef __MOISTURE_SENSOR_H__
#define __MOISTURE_SENSOR_H__


#include "esp_err.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"


#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_CHANNEL ADC1_CHANNEL_5
#define ADC_ATTEN ADC_ATTEN_DB_11

#define NUM_SAMPLES 50


int get_sample();

float get_humidity();

esp_err_t init_adc_config();


#endif