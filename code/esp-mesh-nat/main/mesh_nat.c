#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include <tcpip_adapter.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define MT_SIZE 10
#define MIN_PORT 1024

#define RX_BUF_SIZE  20
#define TX_BUF_SIZE  20
#define MATCHING_TABLE_SIZE 10

#define INTERNAL_TIMEOUT portMAX_DELAY

#define DEST_ADDR "192.168.0.102"
#define DESR_PORT 5002

#define IS_SOCK_ADDR_LEN_VALID(namelen)  ((namelen) == sizeof(struct sockaddr_in)) // namelen longueur
#define IS_SOCK_ADDR_TYPE_VALID(name)    ((name)->sa_family == AF_INET) //name sockaddr
#define IS_SOCK_ADDR_ALIGNED(name)      ((((mem_ptr_t)(name)) % 4) == 0)


static int currentPort = 1024; // max = 65535

static const char *TAG = "mesh_main";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};

struct record{
    int sock;
    uint8_t *addr;
};

static struct record* matching_table[MATCHING_TABLE_SIZE]; // on autorise 10 sockets max

static fd_set readSet;

static int maxSock = 0;

u32_t *myIP=NULL;


void printData(uint8_t *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}

//envoit depuis interne pour externe/interne
void esp_mesh_internal_tx(void *arg){
    esp_err_t err;

    static uint8_t tx_buf[TX_BUF_SIZE]= {238, 238, 238, 238, 238, 
        238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238};

    bool is_running = true;

    /* mesh data */
    mesh_data_t mesh_data;
    mesh_data.data = tx_buf;
    mesh_data.size = sizeof(tx_buf);
    mesh_data.proto = MESH_PROTO_BIN;

    mesh_addr_t to;
    ip4_addr_t address;
    address.addr = ipaddr_addr(DEST_ADDR);
    to.mip.ip4 = address;
    to.mip.port= DESR_PORT;



    while(is_running){
        /* to, data, flag, opt, opt_count
         * to is set to NULL for root
         **/
        //err = esp_mesh_send(NULL, &mesh_data, 0, NULL, 0);
        err = esp_mesh_send(&to, &mesh_data, MESH_DATA_TODS, NULL, 0);// to external ip
        if(err == ESP_OK){
            ESP_LOGI(TAG, "data sent!");
            //sent++;
        }else{
            ESP_LOGE(TAG, "data was not sent");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        is_running = false;
    }
    vTaskDelete(NULL);// delete the task
}

//recoit en interne
void esp_mesh_internal_rx(void *arg){
    esp_err_t err;

    uint8_t rx_buf[RX_BUF_SIZE]={0,}; // buffer
    
    mesh_addr_t from;// source address
    
    int flag = 0;

    /* mesh data */
    mesh_data_t data;
    data.data = rx_buf;
    data.size = sizeof(rx_buf);

    bool is_running = true;

    while(is_running){
        /*from, data, timeout, flag, opt, opt_count*/
        err = esp_mesh_recv(&from, &data, INTERNAL_TIMEOUT, &flag, NULL, 0);
        if(err == ESP_OK){
            ESP_LOGI(TAG, "data received");
            printData(rx_buf, RX_BUF_SIZE); 
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
    vTaskDelete(NULL);// delete the task
}

//recoit depuis interne et retransmet vers externe
//for root
void esp_mesh_external_tx(void *arg){//send
    esp_err_t mesh_err;
    int err;

    uint8_t rx_buf[RX_BUF_SIZE]={0,};
    
    char addr_str[128];

    /* mesh data */
    mesh_data_t mesh_data;
    mesh_data.data = rx_buf;
    mesh_data.size = sizeof(rx_buf);

    /* src and dest MESH addr */
    mesh_addr_t mesh_from_addr;
    mesh_addr_t mesh_to_addr;

    /* src and dest IP addr */
    struct sockaddr_in ip_from_addr; // root ip address
    struct sockaddr_in ip_to_addr;

    /* set ip addr family to AF_INET (for IPv4) */
    ip_from_addr.sin_family = AF_INET;
    ip_to_addr.sin_family = AF_INET;

    /* set ip source */
    tcpip_adapter_ip_info_t ipInfo;
    esp_err_t r = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    if (r != ESP_OK){
        ESP_LOGE(TAG, "can't get info");
    }  
    memcpy((u32_t *) &ip_from_addr.sin_addr, &ipInfo.ip.addr, sizeof(ipInfo.ip.addr));

    int sock; //socket

    int flag = 0;

    bool is_running = true;
    

    while(is_running){
        
        /* from, to, data, timeout, flag, ..., ... */
        mesh_err = esp_mesh_recv_toDS(&mesh_from_addr, &mesh_to_addr, &mesh_data, INTERNAL_TIMEOUT, &flag, NULL, 0);
        
        if(mesh_err != ESP_OK){
            ESP_LOGE(TAG, "error at esp_mesh_recv_toDS");
            continue;
        }

        ESP_LOGI(TAG, "data received for external");
        printData(rx_buf, RX_BUF_SIZE);

        /* set port and ip for destination */
        ip_to_addr.sin_port = htons(mesh_to_addr.mip.port);
        memcpy((u32_t *) &ip_to_addr.sin_addr, &mesh_to_addr.mip.ip4.addr, 
            sizeof(mesh_to_addr.mip.ip4.addr));

        
        struct record* item=NULL;
        int i=0;
        for(i=0; i<MATCHING_TABLE_SIZE; i++){
            if(matching_table[i]==NULL){
                break;
            }
            if(!memcmp(matching_table[i]->addr, mesh_from_addr.addr, sizeof(mesh_from_addr.addr))){//socket for node already exist
                item = matching_table[i];
                break;
            }
        }

        if(item !=NULL){
            /* set port and ip for source */
            ESP_LOGI(TAG, "record already exist");
            ip_from_addr.sin_port = htons(i+MIN_PORT);
            
        }else{
            ESP_LOGI(TAG, "record don't exist");
            ip_from_addr.sin_port = htons(currentPort);
            inet_ntoa_r(ip_from_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
            ESP_LOGI(TAG, "create new socket for port %d and addr %s", currentPort, addr_str);
            
            //########### create socket #################
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if(sock < 0){
                ESP_LOGE(TAG, "unable to create socket");
                break;
            }
            ESP_LOGI(TAG, "socket created");

            if(bind(sock, (struct sockaddr *)&ip_from_addr, sizeof(ip_from_addr)) < 0){
                ESP_LOGE(TAG, "unable to bind socket errno: %d", errno);

                shutdown(sock, 0);
                close(sock);
                break;
            }
            if(connect(sock,(struct sockaddr *)&ip_to_addr, sizeof(ip_to_addr)) < 0){
                ESP_LOGE(TAG, "unable to connect socket");
                shutdown(sock, 0);
                close(sock);
                break;
            }
            if(sock > maxSock){
                maxSock = sock;
            }
            FD_SET(sock, &readSet);// add sock to readSet
            //##########################################
            if(sock < 0){
                continue;
            }
            ESP_LOGI(TAG, "socket bound and connected");
            item  = (struct record*) malloc(sizeof(struct record));
            item->sock = sock;
            item->addr = mesh_from_addr.addr;
            matching_table[currentPort-MIN_PORT] = item;
            currentPort++;
        }
        err = send(item->sock, mesh_data.data, mesh_data.size, 0);
        if(err < 0){
            ESP_LOGE(TAG, "unable to send data");
        }else{
            inet_ntoa_r(ip_to_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
            ESP_LOGI(TAG, "Message sent to %s",addr_str);
        }
        
    }
    vTaskDelete(NULL);
}

//recoit depuis externe pour interne
//for root
void esp_mesh_external_rx(void *arg){//receive
    
    uint8_t rx_buf[RX_BUF_SIZE]={0,}; // buffer
    int recv_value, sock;
    bool is_running = true;
    struct sockaddr_in src;
    socklen_t sockLen;
    ESP_LOGE(TAG, "R1: %d",sizeof(readSet));

    while(is_running){
        ESP_LOGE(TAG, "R1B");
        if(select(maxSock, &readSet, NULL, NULL, NULL) < 0){
            ESP_LOGE(TAG, "select error");
        }
        ESP_LOGE(TAG, "R1C");
        for(int i=0; i<MATCHING_TABLE_SIZE; i++){
            if(matching_table[i]==NULL){
                break;
            }
            sock = matching_table[i]->sock;
            if(FD_ISSET(sock, &readSet)){
                ESP_LOGE(TAG, "R2");
                recv_value = recvfrom(sock, rx_buf, sizeof(rx_buf), 0, &src, &sockLen);
                ESP_LOGE(TAG, "R3");
                if(recv_value == 0){
                    ESP_LOGE(TAG, "R4");
                    ESP_LOGI(TAG, "socket %d closed by peer", sock);
                    shutdown(sock, 0);
                    close(sock);
                    FD_CLR(sock, &readSet);
                }else if(recv_value < 0){
                    ESP_LOGE(TAG, "R5");
                    ESP_LOGE(TAG, "recv error");
                }else{
                    ESP_LOGE(TAG, "R6");
                    ESP_LOGI(TAG, "receveid %d bytes for port %d", recv_value, src.sin_port);
                    printData(rx_buf, recv_value);
                    
                    
                }
            }
        }
    }

}

void start_p2p_comm(void){
    static bool is_comm_p2p_started = false;
    
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        if(esp_mesh_is_root()){
            xTaskCreate(esp_mesh_internal_rx, "IRX", 3072, NULL, 5, NULL);
        }else{
        xTaskCreate(esp_mesh_internal_rx, "IRX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_internal_tx, "ITX", 3072, NULL, 5, NULL);
        }
    }

}

void start_external_comm(void){
    static bool is_comm_external_started = false;

    if (!is_comm_external_started && esp_mesh_is_root()) {
        is_comm_external_started = true;
        xTaskCreate(esp_mesh_external_rx, "ERX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_external_tx, "ETX", 3072, NULL, 5, NULL);
    }
}


void mesh_event_handler(mesh_event_t event)
{
    //ensemble d'adresse mesh union(mac address, mip address mip_t(ipv4, port))
    mesh_addr_t id = {0,};// id du reseau mesh
    ESP_LOGD(TAG, "esp_event_handler:%d", event.id);

    switch (event.id) {
    case MESH_EVENT_STARTED:
        esp_mesh_get_id(&id);
        ESP_LOGI(TAG, "<MESH_EVENT_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        break;
    case MESH_EVENT_STOPPED:
        ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
        break;
    case MESH_EVENT_CHILD_CONNECTED:
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 event.info.child_connected.aid,
                 MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 event.info.child_disconnected.aid,
                 MAC2STR(event.info.child_disconnected.mac));
        break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 event.info.no_parent.scan_times);
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        esp_mesh_get_id(&id);
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>");

        if (esp_mesh_is_root()) {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        start_p2p_comm();
        break;
    case MESH_EVENT_PARENT_DISCONNECTED:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 event.info.disconnected.reason);

        break;
    case MESH_EVENT_LAYER_CHANGE:
        ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>");
        break;
    case MESH_EVENT_ROOT_ADDRESS:// on a l'adresse de root
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(event.info.root_addr.addr));
        break;
    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        myIP = &event.info.got_ip.ip_info.ip.addr;
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&event.info.got_ip.ip_info.ip),
                 IP2STR(&event.info.got_ip.ip_info.netmask),
                 IP2STR(&event.info.got_ip.ip_info.gw));
        start_external_comm();

        break;
    case MESH_EVENT_ROOT_LOST_IP:
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_LOST_IP>");
        break;
    case MESH_EVENT_VOTE_STARTED:
        /*the process of voting a new root is started either by children or by the root*/
        ESP_LOGI(TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 event.info.vote_started.attempts,
                 event.info.vote_started.reason,
                 MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
        ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 event.info.switch_req.reason,
                 MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        /* new root */
        break;
    case MESH_EVENT_TODS_STATE:
        ESP_LOGI(TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
                 event.info.toDS_state);
        break;
    case MESH_EVENT_ROOT_FIXED:
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(event.info.root_conflict.addr),
                 event.info.root_conflict.rssi,
                 event.info.root_conflict.capacity);
        break;
    case MESH_EVENT_CHANNEL_SWITCH:
        ESP_LOGI(TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", event.info.channel_switch.channel);
        break;
    case MESH_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 event.info.scan_done.number);
        break;
    case MESH_EVENT_NETWORK_STATE:
        ESP_LOGI(TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 event.info.network_state.is_rootless);
        break;
    case MESH_EVENT_STOP_RECONNECTION:
        ESP_LOGI(TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        break;
    case MESH_EVENT_FIND_NETWORK:
        ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 event.info.find_network.channel, MAC2STR(event.info.find_network.router_bssid));
        break;
    case MESH_EVENT_ROUTER_SWITCH:
        ESP_LOGI(TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 event.info.router_switch.ssid, event.info.router_switch.channel, MAC2STR(event.info.router_switch.bssid));
        break;
    default:
        ESP_LOGI(TAG, "unknown id:%d", event.id);
        break;
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());

    /*  tcpip initialization */
    tcpip_adapter_init();

    /* initializes the set of descriptors */
    FD_ZERO(&readSet);
    
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
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, "toto",
           strlen("toto"));
    
    /* set mesh configuration */
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    
    /*  disable crypto function */
    esp_mesh_set_ie_crypto_funcs(NULL);
    
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

}