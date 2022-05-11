#include "mesh_control.h"

/*******************************************************
 *                Variable Definitions
 *******************************************************/

const uint8_t MESH_ID[6] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
uint8_t tx_buf[TX_SIZE] = {
    0,
};
uint8_t rx_buf[RX_SIZE] = {
    0,
};
bool is_running = true;
bool is_mesh_connected = false;
mesh_addr_t mesh_parent_addr;

esp_netif_t *netif_sta = NULL;

/*******************************************************
 *                Function Definitions
 *******************************************************/

bool send_to_mqtt(struct Packet data)
{
  char buffer[50];
  if (data.type == Humidity)
  {

    snprintf(buffer, sizeof buffer, "%d", data.data);
    publish_data("sensor/dth22/humidity", buffer);
    return true;
  }

  if (data.type == Temperature)
  {
    snprintf(buffer, sizeof buffer, "%d", data.data);
    publish_data("sensor/dth22/temperature", buffer);
    return true;
  }

  if (data.type == Soil_Humidity)
  {
    snprintf(buffer, sizeof buffer, "%d", data.data);
    publish_data("sensor/dth22/soil_humidity", buffer);
    return true;
  }
  return false;
}

void send_mesh_data(struct Packet data)
{

  esp_err_t err;
  // convierto el paquete en bytes
  char buffer[sizeof(data)];
  memcpy(buffer, &data, sizeof(data));

  // arma el paquete para enviar
  mesh_data_t packet;
  packet.data = (uint8_t)buffer;
  packet.size = sizeof(data);
  packet.proto = MESH_PROTO_BIN;
  packet.tos = MESH_TOS_P2P;

  // dirección a la que se envia
  // mesh_addr_t root_addrs;
  // memset(&root_addrs, 0, sizeof(mesh_addr_t));

  // esto puede ser más optimo.
  //  verifico si es el nodo raiz y que tipo de paquete es para saber si mandarlo a mqtt o a los demas nodo o al nodo raiz
  if (esp_mesh_is_root())
  {

    if (send_to_mqtt(data))
    {
      return;
    }

    // el paquete es del tipo de configuración, entonces lo envio a todos los nodos de la red
    mesh_addr_t route_table[MESH_ROUTE_TABLE_SIZE];
    int route_table_size = 0;
    esp_mesh_get_routing_table((mesh_addr_t *)&route_table, MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

    for (int i = 0; i < route_table_size; i++)
    {
      err = esp_mesh_send(&route_table[i], &packet, MESH_DATA_P2P, NULL, 0);
    }
  }
  else
  {
    // este es el caso de que no se el nodo raiz
    // envio los datos al raiz
    err = esp_mesh_send(NULL, &packet, 0, NULL, 0);
  }
}

// envia datos en la red mesh
/*
void esp_mesh_p2p_tx_main(void *arg)
{
  int i;
  esp_err_t err;
  int send_count = 0;
  mesh_addr_t root_addrs;
  int route_table_size = 0;
  mesh_data_t data;
  data.data = tx_buf;
  data.size = sizeof(tx_buf);
  data.proto = MESH_PROTO_BIN;
  data.tos = MESH_TOS_P2P;
  is_running = true;
  memset(&root_addrs, 0, sizeof(mesh_addr_t));
  while (is_running)
  {

    ESP_LOGI(MESH_TAG, "layer:%d, rtableSize:%d, %s", mesh_layer,
             esp_mesh_get_routing_table_size(),
             (is_mesh_connected && esp_mesh_is_root()) ? "ROOT" : is_mesh_connected ? "NODE"
                                                                                    : "DISCONNECT");
    vTaskDelay(5000 / portTICK_RATE_MS);
  }
  vTaskDelete(NULL);
}
*/

// recibe datos en la red mesh
// aquí solo debería recibir packetes de configuración o paquetes de datos si es el nodo raiz
void esp_mesh_p2p_rx_main(void *arg)
{
  int recv_count = 0;
  esp_err_t err;
  mesh_addr_t from;
  int send_count = 0;
  mesh_data_t data;
  int flag = 0;
  data.data = rx_buf;
  data.size = RX_SIZE;
  is_running = true;

  while (is_running)
  {
    data.size = RX_SIZE;
    // recibo el paquete
    err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
    if (err != ESP_OK || !data.size)
    {
      char buffer[RX_SIZE];
      // convierto el paquete a la structura
      struct Packet tmp;
      memcpy(&tmp, buffer, sizeof(data.size));

      if (esp_mesh_is_root())
      {
        // es el nodo red y recibí paquetes de datos. los envio al mqtt
        if (send_to_mqtt(tmp))
        {
          continue;
        }
      }
      else
      {
        // se reciben paquetes de configuración
        if (tmp.type == Change_Diference)
        {

          continue;
        }

        if (tmp.type == Change_Time)
        {

          continue;
        }

        if (tmp.type == Restart)
        {
          continue;
        }
      }

      continue;
    }

    vTaskDelay(1000 / portTICK_RATE_MS);
  }
  vTaskDelete(NULL);
}

void process_packet(struct Packet packet)
{
}

esp_err_t esp_mesh_comm_p2p_start(void)
{
  bool is_comm_p2p_started = false;
  if (!is_comm_p2p_started)
  {
    is_comm_p2p_started = true;
    // xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
    xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
  }
  return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
  mesh_addr_t id = {
      0,
  };
  uint16_t last_layer = 0;

  switch (event_id)
  {
  case MESH_EVENT_STARTED:
  {
    esp_mesh_get_id(&id);
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "", MAC2STR(id.addr));
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
  case MESH_EVENT_CHILD_CONNECTED:
  {
    mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
             child_connected->aid,
             MAC2STR(child_connected->mac));
  }
  break;
  case MESH_EVENT_CHILD_DISCONNECTED:
  {
    mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
             child_disconnected->aid,
             MAC2STR(child_disconnected->mac));
  }
  break;
  case MESH_EVENT_ROUTING_TABLE_ADD:
  {
    mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
    ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
             routing_table->rt_size_change,
             routing_table->rt_size_new, mesh_layer);
  }
  break;
  case MESH_EVENT_ROUTING_TABLE_REMOVE:
  {
    mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
    ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
             routing_table->rt_size_change,
             routing_table->rt_size_new, mesh_layer);
  }
  break;
  case MESH_EVENT_NO_PARENT_FOUND:
  {
    mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
             no_parent->scan_times);
  }
  /* TODO handler for the failure */
  break;
  case MESH_EVENT_PARENT_CONNECTED:
  {
    mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
    esp_mesh_get_id(&id);
    mesh_layer = connected->self_layer;
    memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR ", duty:%d",
             last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
             esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                               : "",
             MAC2STR(id.addr), connected->duty);
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
  case MESH_EVENT_ROOT_ADDRESS:
  {
    mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
             MAC2STR(root_addr->addr));
  }
  break;
  case MESH_EVENT_VOTE_STARTED:
  {
    mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:" MACSTR "",
             vote_started->attempts,
             vote_started->reason,
             MAC2STR(vote_started->rc_addr.addr));
  }
  break;
  case MESH_EVENT_VOTE_STOPPED:
  {
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
    break;
  }
  case MESH_EVENT_ROOT_SWITCH_REQ:
  {
    mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR "",
             switch_req->reason,
             MAC2STR(switch_req->rc_addr.addr));
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
  case MESH_EVENT_TODS_STATE:
  {
    mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
  }
  break;
  case MESH_EVENT_ROOT_FIXED:
  {
    mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
             root_fixed->is_fixed ? "fixed" : "not fixed");
  }
  break;
  case MESH_EVENT_ROOT_ASKED_YIELD:
  {
    mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
    ESP_LOGI(MESH_TAG,
             "<MESH_EVENT_ROOT_ASKED_YIELD>" MACSTR ", rssi:%d, capacity:%d",
             MAC2STR(root_conflict->addr),
             root_conflict->rssi,
             root_conflict->capacity);
  }
  break;
  case MESH_EVENT_CHANNEL_SWITCH:
  {
    mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
  }
  break;
  case MESH_EVENT_SCAN_DONE:
  {
    mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
             scan_done->number);
  }
  break;
  case MESH_EVENT_NETWORK_STATE:
  {
    mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
             network_state->is_rootless);
  }
  break;
  case MESH_EVENT_STOP_RECONNECTION:
  {
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
  }
  break;
  case MESH_EVENT_FIND_NETWORK:
  {
    mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR "",
             find_network->channel, MAC2STR(find_network->router_bssid));
  }
  break;
  case MESH_EVENT_ROUTER_SWITCH:
  {
    mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, " MACSTR "",
             router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
  }
  break;
  case MESH_EVENT_PS_PARENT_DUTY:
  {
    mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
  }
  break;
  case MESH_EVENT_PS_CHILD_DUTY:
  {
    mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
    ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, " MACSTR ", duty:%d", ps_duty->child_connected.aid - 1,
             MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
  }
  break;
  default:
    ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
    break;
  }
}
#ifdef CONNECT_WIFI_ROUTER
void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
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
  memcpy((uint8_t *)&cfg.router.password, ROUTER_PASSWD,
         strlen(ROUTER_PASSWD));
#endif
  /* mesh softAP */
  err = esp_mesh_set_ap_authmode(MESH_AP_AUTHMODE);
  cfg.mesh_ap.max_connection = MESH_AP_CONNECTIONS;
  memcpy((uint8_t *)&cfg.mesh_ap.password, MESH_AP_PASSWD,
         strlen(MESH_AP_PASSWD));

  err = esp_mesh_set_config(&cfg);
  /* mesh start */
  err = esp_mesh_start();

  ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s<%d>%s, ps:%d\n", esp_get_minimum_free_heap_size(),
           esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
           esp_mesh_get_topology(), esp_mesh_get_topology() ? "(chain)" : "(tree)", esp_mesh_is_ps_enabled());

  return err;
}