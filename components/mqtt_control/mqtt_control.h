#ifndef __MQTT_CONTROL_H__
#define __MQTT_CONTROL_H__

#include "esp_err.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_mesh.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "string.h"
#include <stdint.h>
#include <stddef.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>


#define MQTT_URI "mqtts://redes.danielrojas.website:8883"
#define MQTT_USER "test1"
#define MQTT_PASSWORD "12345678"
#define MAX_MSG_LEN 1024


esp_mqtt_client_handle_t client;

char incoming_topic[MAX_MSG_LEN];
char incoming_msg[MAX_MSG_LEN];

bool is_connected;

void (*process_data_prt)(const char *topic, const char *msg);

void mqtt_app_start(void);

void publish_data(const char* topic, const char* message);

void subscribe_topic(const char *topic);

void mqtt_topics_handler(esp_mqtt_event_handle_t client);

#endif