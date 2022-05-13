#include "mesh_control.h"
#include "main.h"

const uint8_t MESH_ID[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
struct Packet tx_buf;
struct Packet rx_buf;

bool is_running = true;
bool is_mesh_connected = false;
bool is_wifi_connected = false;
mesh_addr_t mesh_parent_addr;
mesh_addr_t my_addr;
esp_netif_t *netif_sta = NULL;

bool get_is_wifi_connected()
{
  return is_wifi_connected;
}

bool get_is_mesh_connected()
{
  return is_mesh_connected;
}

bool get_is_root()
{
  return esp_mesh_is_root();
}

// envia paquetes
void esp_mesh_p2p_tx_main(float value, Type type)
{
  mesh_addr_t route_table[MESH_ROUTE_TABLE_SIZE];
  int route_table_size = 0;
  int i;
  esp_mesh_get_routing_table((mesh_addr_t *)&route_table,
                             MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

  esp_err_t err;
  mesh_data_t data;

  ESP_LOGI(MESH_TAG, "Packet value: %f with type: %i", value, type);

  // inserto los valores al paquete
  tx_buf.value = value;
  tx_buf.type = type;

  // genero el paquete
  data.data = (uint8_t *)&tx_buf;
  data.size = sizeof(tx_buf);
  data.proto = MESH_PROTO_BIN;
  data.tos = MESH_TOS_P2P;

  char buffer[64];
  snprintf(buffer, sizeof buffer, "%f", tx_buf.value);

  if (esp_mesh_is_root())
  {
    // en caso de ser root mandar directamente al mqtt

    // en caso de que los mensajes fuera de configuración
    if (tx_buf.type == Soil_Humidity)
    {
      publish_data("tec/soil/humidity", buffer);
      return;
    }

    if (tx_buf.type == Humidity)
    {
      publish_data("tec/dht22/humidity", buffer);
      return;
    }

    if (tx_buf.type == Temperature)
    {
      publish_data("tec/dht22/temperature", buffer);
      return;
    }

    for (i = 0; i < route_table_size; i++)
    {
      esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0);
      ESP_LOGI(MESH_TAG, "Sending to:" MACSTR "", MAC2STR(route_table[i].addr));
    }

    if (tx_buf.type == Restart)
    {
      ESP_LOGI(MESH_TAG, "Restarting...");
      esp_restart();
      return;
    }
  }

  // en caso de ser un nodo hoja enviar directamente al root
  err = esp_mesh_send(NULL, &data, MESH_DATA_P2P, NULL, 0);
}

// recibe paquetes. se mantiene escuchando siempre
void esp_mesh_p2p_rx_main(void *arg)
{
  esp_err_t err;
  mesh_addr_t from;
  mesh_data_t data;
  int flag = 0;
  data.data = (uint8_t *)&rx_buf;
  data.size = RX_SIZE;
  is_running = true;
  char buffer[64];

  while (is_running)
  {
    data.size = RX_SIZE;
    // recibo el paquete
    err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
    if (err != ESP_OK || !data.size)
    {
      ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
      continue;
    }
    // aqui solo llega si hay un paquete
    ESP_LOGI(MESH_TAG, "Valor del paquete %f, Tipo: %i", rx_buf.value, rx_buf.type);
    ESP_LOGI(MESH_TAG, "Es root:%i", esp_mesh_is_root());
    if (esp_mesh_is_root())
    {
      ESP_LOGI(MESH_TAG, "Sending to MQTT");
      snprintf(buffer, sizeof buffer, "%f", rx_buf.value);
      // en caso de ser root mandar los datos directamente al mqtt
      // ya que aqui solo llegarían los datos de humedad y temperatura de los nodos

      if (rx_buf.type == Soil_Humidity)
      {
        publish_data("tec/soil/humidity", buffer);
        continue;
      }

      if (rx_buf.type == Humidity)
      {
        publish_data("tec/dht22/humidity", buffer);
        continue;
      }

      if (rx_buf.type == Temperature)
      {
        publish_data("tec/dht22/temperature", buffer);
        continue;
      }

      continue;
    }
    ESP_LOGI(MESH_TAG, "En nodo");
    // en caso de ser nodo lo que se recibe son paquetes de configuracion
    if (rx_buf.type == Change_Diference)
    {
      ESP_LOGI(MESH_TAG, "Colocando el valor");
      set_capture_variation(rx_buf.value);
      continue;
    }

    if (rx_buf.type == Change_Time)
    {
      ESP_LOGI(MESH_TAG, "Colocando el valor");
      set_capture_frecuency(rx_buf.value);
      continue;
    }

    if (rx_buf.type == Restart)
    {
      esp_restart();
      continue;
    }
  }
  vTaskDelete(NULL);
}

esp_err_t esp_mesh_comm_p2p_start(void)
{
  static bool is_comm_p2p_started = false;
  if (!is_comm_p2p_started)
  {
    is_comm_p2p_started = true;
    xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
  }
  return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  uint16_t last_layer = 0;

  switch (event_id)
  {
  case MESH_EVENT_STARTED:
  {
    esp_mesh_get_id(&my_addr);
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "", MAC2STR(my_addr.addr));
    is_mesh_connected = false;
    mesh_layer = esp_mesh_get_layer();
  }
  break;
  case MESH_EVENT_STOPPED:
  {
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
    is_mesh_connected = false;
    mesh_layer = esp_mesh_get_layer();
  }
  break;
  case MESH_EVENT_PARENT_CONNECTED:
  {
    mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
    esp_mesh_get_id(&my_addr);
    mesh_layer = connected->self_layer;
    memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR ", duty:%d",
             last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
             esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                               : "",
             MAC2STR(my_addr.addr), connected->duty);
    last_layer = mesh_layer;

    is_mesh_connected = true;
    if (esp_mesh_is_root())
    {
      esp_netif_dhcpc_start(netif_sta);
    }
    esp_mesh_comm_p2p_start();
  }
  break;
  case MESH_EVENT_PARENT_DISCONNECTED:
  {
    mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
             disconnected->reason);
    is_mesh_connected = false;

    mesh_layer = esp_mesh_get_layer();
  }
  break;
  case MESH_EVENT_LAYER_CHANGE:
  {
    mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
    mesh_layer = layer_change->new_layer;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
             last_layer, mesh_layer,
             esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                               : "");
    last_layer = mesh_layer;
  }
  break;
  case MESH_EVENT_ROOT_SWITCH_ACK:
  {
    /* new root */
    mesh_layer = esp_mesh_get_layer();
    esp_mesh_get_parent_bssid(&mesh_parent_addr);
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR "", mesh_layer, MAC2STR(mesh_parent_addr.addr));
  }
  break;
  }
}

#ifdef CONNECT_WIFI_ROUTER
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
  is_wifi_connected = true;
}
#endif

esp_err_t iniciar_mesh_red()
{
  esp_err_t err = ESP_OK;
  err = nvs_flash_init();
  /*  tcpip initialization */
  err = esp_netif_init();
  /*  event initialization */
  err = esp_event_loop_create_default();
  /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
  err = esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL);
  /*  wifi initialization */
  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&config);
#ifdef CONNECT_WIFI_ROUTER
  err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);
#endif
  err = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  err = esp_wifi_start();
  /*  mesh initialization */
  err = esp_mesh_init();
  err = esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL);
  /*  set mesh topology */
  err = esp_mesh_set_topology(MESH_TOPOLOGY);
  /*  set mesh max layer according to the topology */
  err = esp_mesh_set_max_layer(MESH_MAX_LAYER);
#ifndef FIX_ROOT
  err = esp_mesh_set_vote_percentage(1);
#endif
  err = esp_mesh_set_xon_qsize(128);

#ifdef FIX_ROOT
  err = esp_mesh_fix_root(1);
#ifdef ROOT
  err = esp_mesh_set_type(MESH_ROOT);
#endif /*ifdef ROOT*/
#endif
  mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
  /* mesh ID */
  memcpy((uint8_t *)&cfg.mesh_id, MESH_ID, 6);
  /* router */
#ifdef CONNECT_WIFI_ROUTER
  cfg.channel = MESH_CHANNEL;
  cfg.router.ssid_len = strlen(ROUTER_SSID);
  memcpy((uint8_t *)&cfg.router.ssid, ROUTER_SSID, cfg.router.ssid_len);
  memcpy((uint8_t *)&cfg.router.password, ROUTER_PASSWD, strlen(ROUTER_PASSWD));
#endif
  /* mesh softAP */
  err = esp_mesh_set_ap_authmode(MESH_AP_AUTHMODE);
  cfg.mesh_ap.max_connection = MESH_AP_CONNECTIONS;
  memcpy((uint8_t *)&cfg.mesh_ap.password, MESH_AP_PASSWD, strlen(MESH_AP_PASSWD));

  err = esp_mesh_set_config(&cfg);
  /* mesh start */
  err = esp_mesh_start();

  ESP_LOGI(
      MESH_TAG,
      "mesh starts successfully, heap:%d, %s<%d>%s, ps:%d\n",
      esp_get_minimum_free_heap_size(),
      esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
      esp_mesh_get_topology(),
      esp_mesh_get_topology() ? "(chain)" : "(tree)",
      esp_mesh_is_ps_enabled());

  return err;
}