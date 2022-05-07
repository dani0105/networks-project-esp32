#include "moisture_sensor.h"

static const char *TAG = "MOISTURE";


// Inicia analog-to-digital converter (adc)
esp_err_t init_adc_config()
{
  esp_err_t err = ESP_OK;
  err = adc1_config_width(ADC_WIDTH);

  if (err != ESP_OK)
  {
    ESP_LOGE("ADC", "Hubo un error al momento de asignar el ancho del adc");
  }

  err = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

  if (err != ESP_OK)
  {
    ESP_LOGE("ADC", "Hubo un error al configurar atenuacion del adc");
  }

  return err;
}

// Se encarga de conseguir una muestra de valores y devolver el promedio
int get_sample_average()
{
  uint16_t index = 0;
  int sum = 0;
  // ciclo para conseguir la sumatoria de todas las muestras
  while (index++ < NUM_SAMPLES)
  {
    // sumo la muestra obtenida
    sum += adc1_get_raw(ADC_CHANNEL); // LEE EN EL PIN 33

    // espero un tiempo antes de conseguir la siguiente
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  // retorno el promedio
  return sum / NUM_SAMPLES;
}

// Obtiene la humedad del sensor
float get_humidity()
{
  int valor_digital = get_sample_average();
  return -0.0444 * valor_digital + 177.78;
}
