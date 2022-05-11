#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
//#include "wifi.h"
#include "mqtt_control.h"
#include "spiffs_control.h"
#include "moisture_sensor.h"
#include "dht22.h"
#include "mesh_control.h"

// constantes
#define WIFI_SSID "Familia RyR"
#define WIFI_PASSWORD "31300128HRDS"

struct data
{
  float temperature;
  float humidity;
  float soil_humidity;
};

// obtiene el tiempo en milisegundos
long get_time()
{
  return esp_timer_get_time() / 1000;
}

// funcion encargada de leer el sensor dht22
void task_capture_data(void *args)
{
  setDHTgpio(4);
  struct data last_data;
  last_data.temperature = 0;
  last_data.humidity = 0;
  last_data.soil_humidity = 0;

  struct data current_data;
  current_data.temperature = 0;
  current_data.humidity = 0;
  current_data.soil_humidity = 0;
  struct Packet packet;
  long last_record = get_time();
  while (1)
  {
    
    int ret = readDHT();
    long current_record = get_time();
    errorHandler(ret);
    current_data.humidity = getHumidity();
    current_data.temperature = getTemperature();
    current_data.soil_humidity = get_soil_humidity();

    if (abs(last_data.temperature - current_data.temperature) > 0.5 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.temperature = current_data.temperature;
      packet.data = current_data.temperature;
      packet.type = Temperature;
      send_mesh_data(packet);
    }

    if (abs(last_data.humidity - current_data.humidity) > 0.5 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.humidity = current_data.humidity;
      packet.data = current_data.humidity;
      packet.type = Humidity;
      send_mesh_data(packet);
    }

    if (abs(last_data.soil_humidity - current_data.soil_humidity) > 0.5 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.soil_humidity = current_data.soil_humidity;
      packet.data = current_data.soil_humidity;
      packet.type = Soil_Humidity;
      send_mesh_data(packet);
    }

  
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

// punto de inicio del programa
void app_main()
{
  mesh_layer = -1;
  iniciar_mesh_red();

  ESP_ERROR_CHECK(nvs_flash_init());

  // configura spiffs
  config_spiffs();

  // Hace la conexión wifi
  // initialize_wifi(WIFI_SSID, WIFI_PASSWORD);
  // wait_wifi_Connection();

  // Inicia analog-to-digital converter (adc)
  init_adc_config();

  // inicia el mqtt
  mqtt_app_start();

  // Mientra no este conectado al mqtt
  while (!is_connected)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // agrega tareas (son como hilos)
  xTaskCreate(task_capture_data, "task_capture", 2048, NULL, 1, NULL);

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}