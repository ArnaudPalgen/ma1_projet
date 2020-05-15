#include <string.h>
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "SOCKET-SELECT";

#define DEST_ADDR "192.168.0.100"
#define DEST_ADDR_B "192.168.0.102"

#define PORT_A 5005
#define PORT_B 5006
#define PORT 5001

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

fd_set set;

int sockA, sockB;

static int retry = 0;
#define max_retry 5



void printArray(char *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}


void receive(void *arg){
    char buffer[1024];
    int e;
    
    while(1){
        if(select(MAX(sockA, sockB)+1, &set, NULL, NULL, NULL) < 0){
            ESP_LOGE(TAG, "select error");
            continue;
        }

        if(FD_ISSET(sockA, &set)){
            ESP_LOGI(TAG, "data available in sockA !");
            e = recv(sockA, buffer, sizeof(buffer), 0);
            if(e == 0){
                ESP_LOGE(TAG, "sock close by peer");
                shutdown(sockA, 0);
                close(sockA);
                FD_CLR(sockA, &set);
            }else if (e < 0){
                ESP_LOGE(TAG, "recv error");
            }else{
                ESP_LOGI(TAG, "receveid %d bytes", e);
                printArray(buffer, e);
            }
        }
        else if(FD_ISSET(sockB, &set)){
            ESP_LOGI(TAG, "data available in sockB !");
            e = recv(sockB, buffer, sizeof(buffer), 0);
            if(e == 0){
                ESP_LOGE(TAG, "sock close by peer");
                shutdown(sockB, 0);
                close(sockB);
                FD_CLR(sockB, &set);
            }else if (e < 0){
                ESP_LOGE(TAG, "recv error");
            }else{
                ESP_LOGI(TAG, "receveid %d bytes", e);
                printArray(buffer, e);
            }
        }
    }

}

void envoi(void *arg){
    FD_ZERO(&set);//vide l'ensemble

    static const char *payloadA = "Hello Home!A";
    static const char *payloadB = "Hello Home!B";
    
    /* src and dest IP addr */
    struct sockaddr_in ip_from_addr;
    struct sockaddr_in ip_to_addrA;
    struct sockaddr_in ip_to_addrB;

    /* from addr */
    ip_from_addr.sin_port = htons(PORT);
    
    tcpip_adapter_ip_info_t ipInfo;
    esp_err_t r = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    if (r != ESP_OK){
        ESP_LOGE(TAG, "can't get info");
    }
    memcpy((u32_t *) &ip_from_addr.sin_addr, &ipInfo.ip.addr, sizeof(ipInfo.ip.addr));

    ip_from_addr.sin_family = AF_INET;

    /* to addrA */
    ip_to_addrA.sin_family = AF_INET;
    ip_to_addrA.sin_addr.s_addr = inet_addr(DEST_ADDR);
    ip_to_addrA.sin_port = htons(PORT_A);

    /* to addrB */
    ip_to_addrB.sin_family = AF_INET;
    ip_to_addrB.sin_addr.s_addr = inet_addr(DEST_ADDR_B);
    ip_to_addrB.sin_port = htons(PORT_B);

    
    int err;

    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;


    /* socket A */
    sockA = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if(sockA < 0){
        ESP_LOGE(TAG, "unable to create socket A");
    }
    ESP_LOGI(TAG, "socket A created");

    FD_SET(sockA, &set); // ajout du socket dans l'ensemble

    err = bind(sockA, (struct sockaddr *)&ip_from_addr, sizeof(ip_from_addr));
    if(err < 0){
        ESP_LOGE(TAG, "unable to bind socket A");
    }
    err = connect(sockA, (struct sockaddr *)&ip_to_addrA, sizeof(ip_to_addrA));
    if(err < 0){
        ESP_LOGE(TAG, "unable to bind connect A");
    }
    // TODO create task receive
    err = sendto(sockA, payloadA, sizeof(payloadA), 0, (struct sockaddr *)&ip_to_addrA, sizeof(ip_to_addrA));
    if(err < 0){
        ESP_LOGE(TAG, "unable to send with socket A");
    }
    vTaskDelay(xDelay);

    
    /* socket B */
    sockB = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if(sockB < 0){
        ESP_LOGE(TAG, "unable to create socket B");
    }
    ESP_LOGI(TAG, "socket B created");

    FD_SET(sockB, &set); // ajout du socket dans l'ensemble

    int flag = 1;
    setsockopt(sockB, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    err = bind(sockB, (struct sockaddr *)&ip_from_addr, sizeof(ip_from_addr));
    
    if(err < 0){
        ESP_LOGE(TAG, "unable to bind socket B %d", errno);
    }
    err = connect(sockB, (struct sockaddr *)&ip_to_addrB, sizeof(ip_to_addrB));
    if(err < 0){
        ESP_LOGE(TAG, "unable to connect socket B");
    }
    
    xTaskCreate(receive, "RX", 3072, NULL, 5, NULL);

    err = sendto(sockB, payloadB, sizeof(payloadB), 0, (struct sockaddr *)&ip_to_addrB, sizeof(ip_to_addrB));
    if(err < 0){
        ESP_LOGE(TAG, "unable to send socket B");
    }
    vTaskDelay(xDelay);

    while(1){
        err = sendto(sockA, payloadA, sizeof(payloadA), 0, (struct sockaddr *)&ip_to_addrA, sizeof(ip_to_addrA));
        vTaskDelay(xDelay);
        err = sendto(sockB, payloadB, sizeof(payloadB), 0, (struct sockaddr *)&ip_to_addrB, sizeof(ip_to_addrB));
        vTaskDelay(xDelay);
    }

    shutdown(sockA, 0);
    close(sockA);
    shutdown(sockB, 0);
    close(sockB);
    vTaskDelete(NULL);

}

static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "STATION STARTED");
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "STA CONNECTED");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG,"got ip: %s\r\n",
            ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xTaskCreate(envoi, "TX", 3072, NULL, 5, NULL);
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
    ESP_LOGI(TAG, "HELLO HOME !");
    ESP_ERROR_CHECK( nvs_flash_init() );

    //wifi
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_MESH_ROUTER_SSID,
            .password = CONFIG_MESH_ROUTER_PASSWD,
        },
    };

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI(TAG, "WIFI STARTED");

}