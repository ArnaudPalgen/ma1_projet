#include <string.h>
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

#define DEST_ADDR "192.168.0.100"
#define PORT 5005

#define RX_BUF_SIZE  20
#define TX_BUF_SIZE  20



static const char *MESH_TAG = "mesh_main";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // 6 octet apparait dans trame wireshark? change dans le code ?
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;

void esp_mesh_tx(void *arg){
    is_running = true;
    esp_err_t err;
    uint8_t tx_buf[TX_BUF_SIZE]= {238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238};
    mesh_data_t data;
    data.data = tx_buf;
    data.size = sizeof(tx_buf); // pas depasser MESH_MPS cad 1472 bytes
    data.proto = MESH_PROTO_BIN;


    mesh_addr_t to;
    ip4_addr_t address;
    address.addr = ipaddr_addr(DEST_ADDR);
    to.mip.ip4 = address;
    to.mip.port=PORT;

    while(is_running){
        if(!esp_mesh_is_root()){
            //err = esp_mesh_send(&mesh_parent_addr, &data, MESH_DATA_P2P, NULL, 0); // parent address
            //err = esp_mesh_send(NULL, &data, 0, NULL, 0); // to root
            err = esp_mesh_send(&to, &data, MESH_DATA_TODS, NULL, 0);// to external ip
            if(err){
                ESP_LOGI(MESH_TAG, "unable to send ...");
            }else{
                ESP_LOGI(MESH_TAG, "message sent");
                vTaskDelay(2500 / portTICK_PERIOD_MS);//500 ms
                continue;
            }
        }else{
            ESP_LOGI(MESH_TAG, "is root -> don't send");
            vTaskDelete(NULL);
        }
    }
    vTaskDelete(NULL);

}

void printArray(uint8_t *array, int size){
	for(int i=0; i<size; i++){
	    printf("%d ",array[i]);
	}
	printf("\n");
}

void esp_mesh_external_rx(void *arg){
    esp_err_t err;

    uint8_t rx_buf[RX_BUF_SIZE]={0,};

    mesh_addr_t mesh_from_addr;
    mesh_addr_t mesh_to_addr;
    mesh_addr_t mesh_toOld_addr;
    mesh_data_t mesh_data;
    mesh_data.data = rx_buf;
    mesh_data.size = sizeof(rx_buf);
    
    int flag = 0;
    int timeout = 5000;

    int sock;
    int sock_error;

    struct sockaddr_in ip_to_addr;
    struct sockaddr_in ip_from_addr;
    
    ip_to_addr.sin_family = AF_INET;
    
    ip_from_addr.sin_family = AF_INET;
    ip_from_addr.sin_port = htons(2002);
    
    tcpip_adapter_ip_info_t ipInfo;
    esp_err_t r = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    if (r != ESP_OK){
        ESP_LOGE(MESH_TAG, "can't get info");
    }
    
    memcpy((u32_t *) &ip_from_addr.sin_addr, &ipInfo.ip.addr, sizeof(ipInfo.ip.addr));

    bool is_running = true;
    int maxAttempt = 10;
    int attempt = 0;

    char addr_str[128];

    err = esp_mesh_recv_toDS(&mesh_from_addr, &mesh_to_addr, &mesh_data, timeout, &flag, NULL, 0);
    if(err == ESP_OK){
        ESP_LOGI(MESH_TAG, "receive external data");
        printArray(rx_buf, RX_BUF_SIZE);
    }
    memcpy(&mesh_toOld_addr, &mesh_to_addr, sizeof(mesh_to_addr));
    is_running = (err == ESP_OK);

    while(is_running && attempt < maxAttempt){
        ip_to_addr.sin_port = htons(mesh_to_addr.mip.port);
        memcpy((u32_t *) &ip_to_addr.sin_addr, &mesh_to_addr.mip.ip4.addr, sizeof(mesh_to_addr.mip.ip4.addr));
        
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);//address family, tcp socket, ip protocol (IPV4)
        
        int e = bind(sock, (struct sockaddr *)&ip_from_addr, sizeof(ip_from_addr));
        if(e < 0){
            ESP_LOGE(MESH_TAG, "can't bind");
        }
        sock_error = connect(sock, (struct sockaddr *)&ip_to_addr, sizeof(ip_to_addr));

        if(sock < 0 || sock_error != 0){
            ESP_LOGE(MESH_TAG, "Unable to create and connect socket %d", sock);
            if(attempt < maxAttempt){
                ESP_LOGE(MESH_TAG, "retry....");
                attempt ++;
                continue;
            }else{
                ESP_LOGE(MESH_TAG, "maximum number of attempts reached");
                break;
            }
        }else{
            ESP_LOGI(MESH_TAG, "Socket created and connected");
            attempt = 0;
        }
        do{
            sock_error = send(sock, mesh_data.data, mesh_data.size, 0);
            if(sock_error < 0){
                ESP_LOGE(MESH_TAG, "Can't send");
            }else{
                inet_ntoa_r(ip_to_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
                ESP_LOGI(MESH_TAG, "Message sent to %s",addr_str);
            }
            
            err = esp_mesh_recv_toDS(&mesh_from_addr, &mesh_to_addr, &mesh_data, timeout, &flag, NULL, 0);
            if(err == ESP_OK){
                ESP_LOGI(MESH_TAG, "receive external data");
                printArray(rx_buf, RX_BUF_SIZE);
            }
            else{
                is_running = false;
                break;
            }


        }while(mesh_toOld_addr.mip.ip4.addr == mesh_toOld_addr.mip.ip4.addr);
        ESP_LOGE(MESH_TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);

}

void esp_mesh_rx(void *arg){
    is_running = true;
    esp_err_t err = ESP_OK;

    mesh_addr_t from;
    
    uint8_t rx_buf[RX_BUF_SIZE]={0,};
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = sizeof(rx_buf);


    while(is_running){
        err = esp_mesh_recv(&from, &data, 5000, &flag, NULL, 0);
        if(err != ESP_OK){
            is_running = false;
            break;
        }
            
        ESP_LOGI(MESH_TAG, "receive internal data");
        printArray(rx_buf, RX_BUF_SIZE);
    }

    if(err == ESP_ERR_MESH_TIMEOUT){
        ESP_LOGE(MESH_TAG, "ERROR: TIMEOUT");
    }
    else if(err == ESP_ERR_MESH_ARGUMENT){
        ESP_LOGE(MESH_TAG, "ERROR: MESH_ARGUMENT");
        ESP_LOGI(MESH_TAG, "MAC: "MACSTR"", MAC2STR(from.addr));
    }
    else if(err == ESP_ERR_MESH_NOT_START){
        ESP_LOGE(MESH_TAG, "ERROR: MESH_NOT_START");
    }
    else if(err == ESP_ERR_MESH_DISCARD){
        ESP_LOGE(MESH_TAG, "ERROR: MESH_DISCARD");
    }
    else if(err != ESP_OK){
        ESP_LOGE(MESH_TAG, "UNKNOWN ERROR");
    }

    vTaskDelete(NULL);

}

esp_err_t esp_mesh_comm_p2p_start(void)
{
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        /*
         * xTaskCreate cree une tache:
         * pointeur vers la fonction d'entree de la tache
         * nom pour la tache
         * ? nombre de mots a allouer
         * un parametre de la tache
         * priorite de la tache de 0 a 7-1 ifdef SMALL_TEST, de 0 a 25-1 sinon
         * ? pour creer la tache en dehors du xTaskCreate
         */
        
        if(esp_mesh_is_root()){
            ESP_LOGI(MESH_TAG, "is root -> prepare to receive...");
            xTaskCreate(esp_mesh_external_rx, "MPRXE", 3072, NULL, 5, NULL);
            xTaskCreate(esp_mesh_rx, "MPRX", 3072, NULL, 5, NULL);
        }else{
            ESP_LOGI(MESH_TAG, "is child -> prepare to send...");
            xTaskCreate(esp_mesh_tx, "MPTX", 3072, NULL, 5, NULL);
        }
        
        
    }
    return ESP_OK;
}

void mesh_event_handler(mesh_event_t event)
{
    //ensemble d'adresse mesh union(mac address, mip address mip_t(ipv4, port))
    mesh_addr_t id = {0,};// id du reseau mesh
    static uint8_t last_layer = 0;
    ESP_LOGD(MESH_TAG, "esp_event_handler:%d", event.id);

    switch (event.id) {
    case MESH_EVENT_STARTED:
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_STOPPED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_CHILD_CONNECTED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 event.info.child_connected.aid,
                 MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 event.info.child_disconnected.aid,
                 MAC2STR(event.info.child_disconnected.mac));
        break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 event.info.no_parent.scan_times);
        /* TODO handler for the failure */
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        esp_mesh_get_id(&id);
        mesh_layer = event.info.connected.self_layer;
        memcpy(&mesh_parent_addr.addr, event.info.connected.connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        //mesh_connected_indicator(mesh_layer); for light
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        esp_mesh_comm_p2p_start();
        break;
    case MESH_EVENT_PARENT_DISCONNECTED:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 event.info.disconnected.reason);
        is_mesh_connected = false;
        //mesh_disconnected_indicator();
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_LAYER_CHANGE:
        mesh_layer = event.info.layer_change.new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        //mesh_connected_indicator(mesh_layer); for light
        break;
    case MESH_EVENT_ROOT_ADDRESS:// on a l'adresse de root
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(event.info.root_addr.addr));
        break;
    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&event.info.got_ip.ip_info.ip),
                 IP2STR(&event.info.got_ip.ip_info.netmask),
                 IP2STR(&event.info.got_ip.ip_info.gw));
        break;
    case MESH_EVENT_ROOT_LOST_IP:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_LOST_IP>");
        break;
    case MESH_EVENT_VOTE_STARTED:
        /*the process of voting a new root is started either by children or by the root*/
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 event.info.vote_started.attempts,
                 event.info.vote_started.reason,
                 MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 event.info.switch_req.reason,
                 MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
        break;
    case MESH_EVENT_TODS_STATE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
                 event.info.toDS_state);
        break;
    case MESH_EVENT_ROOT_FIXED:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(event.info.root_conflict.addr),
                 event.info.root_conflict.rssi,
                 event.info.root_conflict.capacity);
        break;
    case MESH_EVENT_CHANNEL_SWITCH:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", event.info.channel_switch.channel);
        break;
    case MESH_EVENT_SCAN_DONE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 event.info.scan_done.number);
        break;
    case MESH_EVENT_NETWORK_STATE:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 event.info.network_state.is_rootless);
        break;
    case MESH_EVENT_STOP_RECONNECTION:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        break;
    case MESH_EVENT_FIND_NETWORK:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 event.info.find_network.channel, MAC2STR(event.info.find_network.router_bssid));
        break;
    case MESH_EVENT_ROUTER_SWITCH:
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 event.info.router_switch.ssid, event.info.router_switch.channel, MAC2STR(event.info.router_switch.bssid));
        break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event.id);
        break;
    }
}

void app_main(void)
{
    //ESP_ERROR_CHECK(mesh_light_init());// ---
    //ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    /*  tcpip initialization */
    tcpip_adapter_init();
    /* for mesh
     * stop DHCP server on softAP interface by default
     * stop DHCP client on station interface by default
     * */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
//#if 0
//    /* static ip settings */
//    tcpip_adapter_ip_info_t sta_ip;
//    sta_ip.ip.addr = ipaddr_addr("192.168.1.102");
//    sta_ip.gw.addr = ipaddr_addr("192.168.1.1");
//    sta_ip.netmask.addr = ipaddr_addr("255.255.255.0");
//    tcpip_adapter_set_ip_info(WIFI_IF_STA, &sta_ip);
//#endif
    /*  wifi initialization */
    //ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));// ---
    
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    //ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));// ---
    // j'ai un event handler en plus pour les event ip ---
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    //ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));// ---
    //ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));// ---
    //ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));// ---
//#ifdef MESH_FIX_ROOT// ---
//    ESP_ERROR_CHECK(esp_mesh_fix_root(1));// ---
//#endif// ---
    //ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));// ---
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    //cfg.crypto_funcs = NULL;
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* mesh event callback */
    cfg.event_cb = &mesh_event_handler;// ---
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
           strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    //ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));// ---
    cfg.mesh_ap.max_connection = 4;// ---
    memcpy((uint8_t *) &cfg.mesh_ap.password, "toto",
           strlen("toto"));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    esp_mesh_set_ie_crypto_funcs(NULL);
    //esp_mesh_set_ie_crypto_key("cleaes", 4);
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");
}
