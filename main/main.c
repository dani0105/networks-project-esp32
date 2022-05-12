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

  long last_record = get_time();
  while (1)
  {

    int ret = readDHT();
    long current_record = get_time();
    errorHandler(ret);
    current_data.humidity = getHumidity();
    current_data.temperature = getTemperature();
    current_data.soil_humidity = get_soil_humidity();

    if (abs(last_data.temperature - current_data.temperature) > 1 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.temperature = current_data.temperature;

      esp_mesh_p2p_tx_main(current_record, Temperature);
    }

    if (abs(last_data.humidity - current_data.humidity) > 1 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.humidity = current_data.humidity;
      esp_mesh_p2p_tx_main(current_record, Humidity);
    }

    if (abs(last_data.soil_humidity - current_data.soil_humidity) > 1 || (current_record - last_record) > 60000)
    {
      last_record = current_record;
      last_data.soil_humidity = current_data.soil_humidity;
      esp_mesh_p2p_tx_main(current_record, Soil_Humidity);
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

  // mientras no este conectado al wifi o a una red mesh
  while (!get_is_wifi_connected() && !get_is_mesh_connected())
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  ESP_ERROR_CHECK(nvs_flash_init());

  // Inicia analog-to-digital converter (adc)
  init_adc_config();

  // si esta conectado al wifi
  if(get_is_wifi_connected()){
    // inicia el mqtt
    mqtt_app_start();

    // Mientra no este conectado al mqtt
    while (!is_connected)
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  // agrega tareas (son como hilos)
  xTaskCreate(task_capture_data, "task_capture", 2048, NULL, 1, NULL);

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}