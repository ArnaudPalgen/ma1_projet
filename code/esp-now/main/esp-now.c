#include <string.h>
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_now.h"


#define CHANNEL 7
#define ESPNOW_WIFI_MODE WIFI_MODE_STA // mode wifi (sta, ap, sta+ap)
#define ESPNOW_WIFI_IF ESP_IF_WIFI_STA // interface wifi (sta ou ap)

static const char *TAG = "espnow";
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static uint8_t peer_addrA[ESP_NOW_ETH_ALEN] = {0x3C, 0x71, 0xBF, 0x0D, 0x7E, 0x1C};
static uint8_t peer_addrB[ESP_NOW_ETH_ALEN] = {0x3c, 0x71, 0xbf, 0x0d, 0x7e, 0x18};



static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "WiFi started");
        break;
    default:
        break;
    }
    return ESP_OK;
}

void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status){
    ESP_LOGI(TAG, "send callback function");

}

void printArray(uint8_t *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}

void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len){
    ESP_LOGI(TAG, "receive callback function");
    ESP_LOGI(TAG, "DATA LEN%d:", data_len);
    printf("data\n");
    printArray(&data, data_len);
}



void app_main(void){
    nvs_flash_init();

    //Wi-Fi initialisation
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();    
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    //esp-now initialisation
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );
    
    esp_now_peer_info_t peer;
    peer.channel = CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    //3c:71:bf:0d:7e:1c
    //3c:71:bf:0d:7e:18
    //esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6])
    //esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len)
    uint8_t myAddress[6];
    esp_wifi_get_mac(ESPNOW_WIFI_IF, &myAddress);

    const uint8_t data[20]= {238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238};

    if (memcmp(&myAddress, &peer_addrB, sizeof(myAddress)) == 0){
        ESP_LOGI(TAG, "send to a");
        memcpy(peer.peer_addr, peer_addrA, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
        esp_now_send(&peer_addrA, &data, sizeof(data));
    }/*else{
        ESP_LOGI(TAG, "send to b");
        memcpy(peer.peer_addr, peer_addrB, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
        esp_now_send(&peer_addrB, &data, sizeof(data));
    }*/

}