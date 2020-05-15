#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <tcpip_adapter.h>


#define max_retry 5
#define DEST_ADDR "192.168.0.102"
#define DEST_PORT 5005
#define SRC_PORT 5004

static const char *TAG = "SOCKET";
static int retry = 0;


void mySend(){

    /* data to send */
    char dest_addr_str[128];
    static const char *payload = "Hello Home!";

    /* set dest addr */
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr(DEST_ADDR);
    destAddr.sin_port = htons(DEST_PORT);
    destAddr.sin_family = AF_INET;
    inet_ntoa_r(destAddr.sin_addr, dest_addr_str, sizeof(dest_addr_str) - 1);

    /* set src addr */
    struct sockaddr_in srcAddr;
    srcAddr.sin_port = htons(SRC_PORT);
    srcAddr.sin_family = AF_INET;
    
    tcpip_adapter_ip_info_t ipInfo;
    esp_err_t r = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);//get station info
    if (r != ESP_OK){
        ESP_LOGE(TAG, "Unable to get my IP adress");
        return;
    }
    //set srcAddr IP to station IP
    memcpy((u32_t *) &srcAddr.sin_addr, &ipInfo.ip.addr, sizeof(ipInfo.ip.addr));
    char src_addr_str[128];
    inet_ntoa_r(srcAddr.sin_addr, src_addr_str, sizeof(src_addr_str) - 1);

    /* create TCP socket */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(sock < 0){
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    /* bind socket to srcAddr */
    if(bind(sock, (struct sockaddr *)&srcAddr, sizeof(srcAddr)) < 0){
        ESP_LOGE(TAG, "Unable to bind socket ");
        return;
    }
    ESP_LOGI(TAG, "Socket bound to %s:%d", src_addr_str, ntohl(srcAddr.sin_port));
    
    /* connect socket to destAddr */
    if(connect(sock, (struct sockaddr *) &destAddr, sizeof(destAddr)) < 0){
        ESP_LOGE(TAG, "Unable to connect socket to %s", dest_addr_str);
        return;
    }
    ESP_LOGI(TAG, "Socket connected to %s", dest_addr_str);
    
    /* send data */
    if(send(sock, payload, sizeof(payload), 0) < 0){
        ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Message sent");
}


static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "STATION STARTED");
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "STATION CONNECTED");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG,"got ip: %s\r\n",
            ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        mySend();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if(retry < max_retry){
            retry++;
            esp_wifi_connect();
            ESP_LOGI(TAG,"retry to connect to the AP");
        }else{
           ESP_LOGI(TAG,"connect to the AP fail\n"); 
        }
    default:
        break;
    }
    return ESP_OK;
}

void app_main(){
    ESP_ERROR_CHECK(nvs_flash_init());

    /* tcpip initialization*/    
    tcpip_adapter_init();

    /* wifi initialization */ 
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));//set event handler
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = { //set router SSID and password
        .sta = {
            .ssid = CONFIG_ROUTER_SSID,
            .password = CONFIG_ROUTER_PASSWD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); //set wifi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WIFI STARTED");

}