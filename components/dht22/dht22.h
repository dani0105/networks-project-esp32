#ifndef __DHT22_H__
#define __DHT22_H__


#include "esp_err.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "string.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"


#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2


void 	setDHTgpio(int gpio);
void 	errorHandler(int response);
int 	readDHT();
float 	getHumidity();
float 	getTemperature();
int 	getSignalLevel( int usTimeOut, bool state );


#endif