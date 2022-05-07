#include "spiffs_control.h"


static const char *TAG = "SPIFFS";

int config_spiffs()
{
  ESP_LOGI(TAG, "Initializing SPIFFS");

  // crea el achivo de configuracion SPIFFS
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false};

  // inicializa y monta el sistema de archivo SPIFFS
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
      return 0;
    }

    if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
      return 0;
    }
  
    ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    return 0;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    return 0;
  }

  ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  return 1;
  
}