#include "main.h"

#define TAG "MAIN"

float variation = 1;     // esta es la variacion que va a tener el cambio de datos en los sensores.
float frecuency = 60000; // esta es la frecuencia de envio de los datos conseguidos.

//Esta funcion se encarga de actualizar la frecuencia con la que se envian los datos. Recibe un parametro de tiempo tipo float.
void set_capture_frecuency(float value)
{
  frecuency = value;
  if (get_is_root())
  {
    ESP_LOGI(TAG, "Sending %f to another nodes", value);
    esp_mesh_p2p_tx_main(value, Change_Time);
  }
}

//Esta funcion se encarga de actualizar la variacion que debe presentar una variable para envair los datos. Recibe un parametro de tiempo tipo float.
void set_capture_variation(float value)
{
  variation = value;
  if (get_is_root())
  {
    ESP_LOGI(TAG, "Sending %f to another nodes", value);
    esp_mesh_p2p_tx_main(value, Change_Diference);
  }
}

// obtiene el tiempo en milisegundos
long get_time()
{
  return esp_timer_get_time() / 1000;
}

// Funcion encargada de leer el sensor dht22 y el sensor soil moisture_sensor. El parametro que se envia es de referencia y no esta siendo utilizado.
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

  long last_record_temp = get_time();
  long last_record_hum = get_time();
  long last_record_soil = get_time();
  while (1)
  {
    ESP_LOGI(TAG, "Time: %f. Variation %f", frecuency, variation);
    int ret = readDHT();
    long current_record = get_time();

    // si la lectura fue buena
    if (ret == DHT_OK)
    {
      current_data.humidity = getHumidity();
      current_data.temperature = getTemperature();

      //Si el dato actual es mayor a la variacion o si ya paso el tiempo o frecuencia establecida, se envian los datos.
      if (abs(last_data.temperature - current_data.temperature) > variation || (current_record - last_record_temp) > frecuency)
      {
        ESP_LOGI(TAG,"Temperatura: %i",abs(last_data.temperature - current_data.temperature));
        last_record_temp = current_record;
        last_data.temperature = current_data.temperature;

        esp_mesh_p2p_tx_main(current_data.temperature, Temperature);
      }

      //Si el dato actual es mayor a la variacion o si ya paso el tiempo o frecuencia establecida, se envian los datos.
      if (abs(last_data.humidity - current_data.humidity) > variation || (current_record - last_record_hum) > frecuency)
      {
        ESP_LOGI(TAG,"Humedad: %i",abs(last_data.humidity - current_data.humidity));
        last_record_hum = current_record;
        last_data.humidity = current_data.humidity;
        esp_mesh_p2p_tx_main(current_data.humidity, Humidity);
      }
    }

    current_data.soil_humidity = get_soil_humidity();
    // si los valores estan en el rango de los sensores se envian estos datos.
    if (100 > current_data.soil_humidity && current_data.soil_humidity > 0)
    {
      if (abs(last_data.soil_humidity - current_data.soil_humidity) > variation || (current_record - last_record_soil) > frecuency)
      {
        last_record_soil = current_record;
        last_data.soil_humidity = current_data.soil_humidity;
        esp_mesh_p2p_tx_main(current_data.soil_humidity, Soil_Humidity);
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

// punto de inicio del programa
void app_main()
{
  mesh_layer = -1;
  ESP_LOGI(TAG, "Starting MESH red");
  iniciar_mesh_red();
  // mientras no este conectado al wifi o a una red mesh
  ESP_LOGI(TAG, "Wating for MESH");
  while (!get_is_mesh_connected())
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // es el nodo red y debe esperar por el wifi
  if (get_is_root())
  {
    ESP_LOGI(TAG, "Wating for WIFI");
    while (!get_is_wifi_connected())
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  ESP_ERROR_CHECK(nvs_flash_init());

  // Inicia analog-to-digital converter (adc)
  init_adc_config();

  // si esta conectado al wifi es el raiz y debe iniciar el MQTT
  if (get_is_wifi_connected())
  {
    // inicia el mqtt
    ESP_LOGI(TAG, "Starting MQTT conection");
    mqtt_app_start();

    // Mientra no este conectado al mqtt
    while (!is_connected)
    {
      ESP_LOGI(TAG, "Wating for MQTT connection");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Subscribing to topics");
    subscribe_topic("esp32/frecuency");
    subscribe_topic("esp32/restart");
    subscribe_topic("esp32/variation");
  }

  // agrega tareas (son como hilos)
  xTaskCreate(task_capture_data, "task_capture", 2048, NULL, 1, NULL);

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}