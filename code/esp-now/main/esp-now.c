#include <string.h>
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"


#define CHANNEL 7
#define ESPNOW_WIFI_MODE WIFI_MODE_STA // Wi-Fi mode: sta, ap or sta+ap
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA // Wi-Fi interface sta or ap
#define IS_SOURCE false

static const char *TAG = "espnow";

static const uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t myAddress[ESP_NOW_ETH_ALEN];

void printArray(const uint8_t *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Wi-Fi started");
        break;
    default:
        break;
    }
    return ESP_OK;
}

void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status){
    ESP_LOGI(TAG, "send callback function");
}

void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len){
    ESP_LOGI(TAG, "receive %d bytes:", data_len);
    printArray(data, data_len);
}

void esp_now_tx(void *arg){

    const uint8_t data[20]= {238, 238, 238, 238, 238, 238, 238, 
        238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 237};

    bool is_running = true;

    /* create peer broadcast */
    esp_now_peer_info_t peer;
    peer.channel = CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer)); // add peer

    ESP_LOGI(TAG, "send to all nodes");
    while(is_running){
        esp_now_send(&broadcast_addr, &data, sizeof(data));
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // delay the task
    }
    vTaskDelete(NULL); 
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());

    /* tcpip initialization */
    tcpip_adapter_init();
    
    /* create event handler */
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    
    /* Wi-Fi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();    
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    /* ESP-NOW initialization */
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    
    /* get own MAC addr */
    esp_wifi_get_mac(ESPNOW_WIFI_IF, &myAddress);


    if(IS_SOURCE){
        xTaskCreate(esp_now_tx, "NOWTX", 3072, NULL, 5, NULL);
    }
}