#ifndef __MESH_CONTROL_H__
#define __MESH_CONTROL_H__

#include "esp_err.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_mesh.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "string.h"

//#define FIX_ROOT
//#define ROOT

#define CONNECT_WIFI_ROUTER

/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE          (1500)
#define TX_SIZE          (1460)

#define MESH_TAG "mesh_main"


/*
"Mesh Routing Table Size"
range 1 300
default 50
help
    The number of devices over the network(max: 300).
*/
#define MESH_ROUTE_TABLE_SIZE   50
/*
default 0 if MESH_TOPO_TREE
default 1 if MESH_TOPO_CHAIN
*/
#define MESH_TOPOLOGY           0

/*
"Mesh Max Layer"
range 1 25 if MESH_TOPO_TREE
range 1 1000 if MESH_TOPO_CHAIN
default 6   
*/
#define MESH_MAX_LAYER          6
            
        
//Channel 0 14  default 0
#define MESH_CHANNEL            0
#define ROUTER_SSID             "luillidei"
#define ROUTER_PASSWD           "luillidei"

/*
default 0 if WIFI_AUTH_OPEN
default 2 if WIFI_AUTH_WPA_PSK
default 3 if WIFI_AUTH_WPA2_PSK
default 4 if WIFI_AUTH_WPA_WPA2_PSK
*/
#define MESH_AP_AUTHMODE        2

//Connections  0  10  default 6
#define MESH_AP_CONNECTIONS     2

#define MESH_AP_PASSWD          "RedesIoT"


int mesh_layer;
esp_err_t iniciar_mesh_red();

#endif