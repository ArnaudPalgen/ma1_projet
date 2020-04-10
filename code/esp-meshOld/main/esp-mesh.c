#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_wifi_types.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_mesh.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *MESH_TAG = "mesh_main";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};

#define SSID "raspberryWIFI"
#define PASSWD "esp32raspberry"
#define CHANNEL 7

static bool isRoot = false;
static bool isChild = false;
static int mesh_layer = -1;

void send(){
    if(esp_mesh_is_root_fixed() && esp_mesh_get_type() == MESH_LEAF){
        ESP_LOGI(MESH_TAG, "will send !");
        
        /*
        mesh_addr_t to;
        ip4_addr_t address;
        address.addr = ipaddr_addr("192.168.4.1");
        to.mip.ip4 = address;
        to.mip.port=5005;
        */
        
        mesh_data_t mesh_data;
        uint8_t data[1];
        data[0] = 'a';
        mesh_data.data=data;
        mesh_data.size = sizeof(data);
        mesh_data.proto = MESH_PROTO_BIN;
        mesh_data.tos = MESH_TOS_P2P;
        
        ESP_LOGI(MESH_TAG, "BEFORE SEND");
        ESP_ERROR_CHECK(esp_mesh_send(NULL, &mesh_data, MESH_DATA_P2P, NULL, 0));
        ESP_LOGI(MESH_TAG, "SENDED !");
    }else{
        ESP_LOGI(MESH_TAG, "NOT READY !");
        printf("fiexd? %d\n", esp_mesh_is_root_fixed());
        bool isLeaf = (esp_mesh_get_type() == MESH_LEAF);
        printf("type leaf? %d\n", isLeaf);
    }
}

void receive(){
    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    
    while(true){
        ESP_LOGI(MESH_TAG, "BEFORE RECEIVE");
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        ESP_LOGI(MESH_TAG, "AFTER");
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }else{
            printf("DATA! %d:", data.data[0]);
        }
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
       ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
       ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
}


void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    switch (event_id) {
        case MESH_EVENT_CHILD_CONNECTED: {
            ESP_LOGI(MESH_TAG,"child connected");
        }
        break;

        case MESH_EVENT_PARENT_CONNECTED: {
            mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
            mesh_layer = connected->self_layer;
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_PARENT_CONNECTED>layer:%d",mesh_layer);

        }
        break;

        case  MESH_EVENT_VOTE_STARTED: {
            ESP_LOGI(MESH_TAG,"vote started");
        }
        break;

        case MESH_EVENT_VOTE_STOPPED: {
            ESP_LOGI(MESH_TAG,"vote stopped");
        }
        break;

        case MESH_EVENT_ROOT_ADDRESS: {
            ESP_LOGI(MESH_TAG,"got root address !");
            if (esp_mesh_is_root()){
                ESP_LOGI(MESH_TAG, "is root");
                receive();
                isRoot=true;
            }else{
                ESP_LOGI(MESH_TAG, "is child");
                send();
                isChild=true;
            }
            
        }
        break;
    }
}

void app_main(void){
    /* initialisation de la memoire */
    ESP_ERROR_CHECK(nvs_flash_init());
    
    /*  initialisation tcpip dans le cas ou un noeud devient la racine */
    tcpip_adapter_init();
    
    /*
     * STOP le serveur DHCP sur les interfaces softAP et station
     * Dans le cas ou le noeud devient la racine, il devra etre
     * reactive par l'handler correspondant 
     */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));

    /*  initialisation des evenements */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /*  initialisation du Wi-Fi */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    
    /*  enregistrement de l'handler IP */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &mesh_event_handler, NULL));
    
    //ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    
    /* demarrage du wifi */
    ESP_ERROR_CHECK(esp_wifi_start());

    /*  initialisation mesh */
    ESP_ERROR_CHECK(esp_mesh_init());
    
    /*  enregistrement de lhandler esp-mesh  */
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));

    /* Enable the Mesh IE encryption by default */
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT(); // voir dans fichier header
    
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    
    /* canal identique a celui du routeur */
    cfg.channel = CHANNEL;
    
    /* routeur */
    cfg.router.ssid_len = strlen(SSID);
    memcpy((uint8_t *) &cfg.router.ssid, SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, PASSWD,
           strlen(PASSWD));
    
    /* mesh softAP */
    cfg.mesh_ap.max_connection = 4;
    memcpy((uint8_t *) &cfg.mesh_ap.password, "toto",
           strlen("toto"));//essayer de mettre null
    cfg.allow_channel_switch=true;
    cfg.crypto_funcs = NULL; // peut etre payload qui est crypte 
    //regarder la version
    //essayer avec interface de mon pc renseigner sur dongle
    //connecter un deboger a openocd comme gdb
    //esp now
    //a donner: setup adapater wifi avec son modele, schema de cablage

    
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    ESP_ERROR_CHECK(esp_mesh_start());
    /*
    if(esp_mesh_is_root()){
        tcpServerAddr.sin_addr.s_addr = inet_addr(MESH_SERVER_IP);
        tcpServerAddr.sin_family = AF_INET;
        tcpServerAddr.sin_port = htons( MESH_PORT );
        s = socket(AF_INET, SOCK_STREAM, 0);
        connect(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr))
        write(s , message , strlen(message))
        
    }*/
}

/*
import socket

UDP_IP = "192.168.4.1"
UDP_PORT = 5005
MESSAGE = "Hello, World!"

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", MESSAGE
   
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
*/