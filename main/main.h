#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_control.h"
#include "moisture_sensor.h"
#include "dht22.h"
#include "mesh_control.h"

//Se definen las variables para implementarlas en el main.c


//Estructura que contiene temperatura, humedad y humedad del suelo.
struct data
{
  float temperature;
  float humidity;
  float soil_humidity;
} ;

void set_capture_frecuency(float value);
void set_capture_variation(float value);

long get_time();
void task_capture_data(void *args);
void app_main();