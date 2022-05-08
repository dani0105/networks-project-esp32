#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "wifi.h"
#include "mqtt_control.h"
#include "spiffs_control.h"
#include "moisture_sensor.h"
#include "dht22.h"
#include "mesh_control.h"

// constantes
#define WIFI_SSID "Familia RyR"
#define WIFI_PASSWORD "31300128HRDS"

// obtiene el tiempo en milisegundos
long get_time()
{
  return esp_timer_get_time() / 1000;
}

// funcion encargada de leer el sensor dht22
void task_capture_dht22(void *args)
{
  float last_temperature = 0;
  float last_humidity = 0;
  float humidity = 0;
  float temperature = 0;

  long last_record = get_time();

  setDHTgpio(4);
  char temperatureBuffer[64];
  char humidityBuffer[64];
  while (1)
  {
    int ret = readDHT();

    errorHandler(ret);

    humidity = getHumidity();
    temperature = getTemperature();

    if (abs(last_temperature - temperature) > 0.5 || (get_time() - last_record) > 60000)
    {

      int ret = snprintf(temperatureBuffer, sizeof temperatureBuffer, "%f", temperature);
      publish_data("sensor/dth22/temperature", temperatureBuffer);
    }

    if (abs(last_humidity - humidity) > 0.5 || (get_time() - last_record) > 60000)
    {
      int ret = snprintf(humidityBuffer, sizeof humidityBuffer, "%f", humidity);
      publish_data("sensor/dth22/humidity", humidityBuffer);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

// funcion encargada de leer el sensor soil moisture
void task_captura_moisture(void *args)
{
  // Ultimo valor capturado
  float last_value = 0;
  // valor actual
  float current_value = 0;
  // tiempo del ultimo valor capturado
  long last_record = get_time();

  char data[20];

  // ciclo para recolectar por siempre los valores
  while (1)
  {
    current_value = get_humidity();

    // el valor actual es mayor por 0.5 o ha pasado un minuto
    if (abs(last_value - current_value) > 0.5 || (get_time() - last_record) > 60000)
    {
      // guardo los valores actuales
      last_value = current_value;
      last_record = esp_timer_get_time() / 1000;
      sprintf(data, "%.4f", last_value);
      // envio la información al mqtt
      publish_data("sensor/soil_moisture/humidity", data);
    }

    // espero un tiempo antes de hacer la siguiente lectura
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
  initialize_wifi(WIFI_SSID, WIFI_PASSWORD);
  wait_wifi_Connection();

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
  xTaskCreate(task_capture_dht22, "task_dht22", 2048, NULL, 1, NULL);
  xTaskCreate(task_captura_moisture, "task_moisture", 2048, NULL, 2, NULL);

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}