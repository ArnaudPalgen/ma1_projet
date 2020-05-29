#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"

#define RX_BUF_SIZE  20
#define TX_BUF_SIZE  20
#define NBR_TO_SEND  100 // number of packets to send

static const char *TAG = "mesh_internal";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // mesh id


void printArray(uint8_t *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}

void esp_mesh_rx(void *arg){
    esp_err_t err;

    int received = 0; //number of packets received
    uint8_t rx_buf[RX_BUF_SIZE]={0,}; //receive buffer

    mesh_addr_t from; //src addr
    
    int flag = 0;
    
    /* mesh data */
    mesh_data_t data;
    data.data = rx_buf;
    data.size = sizeof(rx_buf);

    while(received < NBR_TO_SEND){
        /* recv
         *  - from addr
         *  - data
         *  - timeout in ms (0:no wait, portMAX_DELAY:wait forever)
         *  - flag
         *  - options
         *  - number of options
         **/
        err = esp_mesh_recv(&from, &data, 5000, &flag, NULL, 0);

        if(err == ESP_OK){
            received ++ ;
            ESP_LOGI(TAG, "data received");
            printArray(rx_buf, RX_BUF_SIZE);
        }
        else if(err == ESP_ERR_MESH_TIMEOUT){
            ESP_LOGE(TAG, "ERROR: TIMEOUT");
        }
        else if(err == ESP_ERR_MESH_ARGUMENT){
            ESP_LOGE(TAG, "ERROR: MESH_ARGUMENT");
        }
        else if(err == ESP_ERR_MESH_NOT_START){
            ESP_LOGE(TAG, "ERROR: MESH_NOT_START");
        }
        else if(err == ESP_ERR_MESH_DISCARD){
            ESP_LOGE(TAG, "ERROR: MESH_DISCARD");
        }
        else{
            ESP_LOGE(TAG, "UNKNOWN ERROR");
        }
    }
    /* all messages have been received -> delete the task */
    vTaskDelete(NULL); 


}

void esp_mesh_tx(void *arg){
    esp_err_t err;

    int sent = 0; //number of packets sent
    static uint8_t tx_buf[TX_BUF_SIZE]= {238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238};
    
    /* mesh data */
    mesh_data_t mesh_data;
    mesh_data.data = tx_buf;
    mesh_data.size = sizeof(tx_buf);
    mesh_data.proto = MESH_PROTO_BIN;

    while(sent < NBR_TO_SEND){
        /* send:
         *  - dest addr
         *  - data: NULL for the root
         *  - flag
         *  - options
         *  - number of options
         **/
        err = esp_mesh_send(NULL, &mesh_data, 0, NULL, 0);
        if(err == ESP_OK){
            ESP_LOGI(TAG, "data sent!");
            sent++;
        }else{
            ESP_LOGE(TAG, "data was not sent");
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS ); // delay the task
    }
    /* all messages have been sent -> delete the task */
    vTaskDelete(NULL);

}

void esp_mesh_comm_p2p_start(){
    static bool p2p_started = false;
    if(!p2p_started){
        if(esp_mesh_is_root()){// root node
            ESP_LOGI(TAG, "is root -> prepare to receive...");
            /* create task to receive */
            xTaskCreate(esp_mesh_rx, "MPRX", 3072, NULL, 5, NULL);
            

        }else{// intermediate or leaf node
            ESP_LOGI(TAG, "is child -> prepare to send...");
            /* create task to send data */
            xTaskCreate(esp_mesh_tx, "MPTX", 3072, NULL, 5, NULL);

        }
    }

}

void mesh_event_handler(mesh_event_t event){
    
    if (event.id == MESH_EVENT_PARENT_CONNECTED){
        if (esp_mesh_is_root()) {
            /* start DHCP server for the root node */
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        esp_mesh_comm_p2p_start();
    }

}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());

    /* tcpip initialization */
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

}