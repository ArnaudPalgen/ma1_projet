#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#define NUMBER_OF_NODES 5

static const char *TAG = "mesh_internal";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // mesh id
static bool is_root = false;

void mesh_event_handler(mesh_event_t event){
    if (event.id == MESH_EVENT_PARENT_CONNECTED){
        if (esp_mesh_is_root()) {
            /* start DHCP server for the root node */
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
            is_root=true;
        }
    }
    if(event.id  == MESH_EVENT_ROUTING_TABLE_ADD){
        if(esp_mesh_get_routing_table_size() == NUMBER_OF_NODES){
            ESP_LOGV(TAG, "network complete after %llx ms", esp_timer_get_time());
        }
    }

}

void app_main(void){
    /* timier initialisation */
    //ESP_ERROR_CHECK(esp_timer_init());
    
    ESP_ERROR_CHECK(nvs_flash_init());

    /* tcpip initialization*/    
    tcpip_adapter_init();
    
    /* stop DHCP server for softAP and station interfaces */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));

    /* wifi initialization */ 
    ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    /*  mesh initialization*/
    ESP_ERROR_CHECK(esp_mesh_init());
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    
    /* event handler */
    cfg.event_cb = &mesh_event_handler;
    
    /* router settings */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
           strlen(CONFIG_MESH_ROUTER_PASSWD));
    
    /* softAP */
    cfg.mesh_ap.max_connection = 4;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));
    
    /* set mesh configuration */
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    
    /*  disable crypto function */
    esp_mesh_set_ie_crypto_funcs(NULL);
    
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

    /*
    esp_mesh_get_layer
    esp_mesh_get_parent_bssid
    esp_mesh_get_vote_percentage
    esp_mesh_get_total_node_num
    esp_mesh_get_routing_table_size
    esp_mesh_get_routing_table
    esp_mesh_get_subnet_nodes_num //nbr de noeuds dans le sous arbre d'un enfant
    esp_mesh_get_subnet_nodes_list
    esp_timer_get_time
    */

}