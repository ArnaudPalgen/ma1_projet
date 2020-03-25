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

//. $HOME/esp/esp-idf/export.sh
// x-terminal-emulator

void app_main(void){
    static const char* TAG = "ESP-SCAN";
    ESP_LOGI(TAG, "Hello World3 !");
    
    
    /* 
    * struct qui contient la configuration wifi
    * WIFI_INIT_CONFIG_DEFAULT retourne une configuration par defaut
    */
    wifi_init_config_t wifi_config =  WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(nvs_flash_init()); // initialise the storage driver for wifi library
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config)); //ESP_ERROR_CHECK afficher l'erreur et abort s'il y en a une
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // on definis le mode: station l'esp va se connecter
    ESP_ERROR_CHECK(esp_wifi_start());// on demarre le wifi

    wifi_scan_config_t scan_config ={
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        //.scan_type = WIFI_SCAN_TYPE_ACTIVE,
        //.scan_time.active.min = 10,
        //.scan_time.active.max = 100,
        //.scan_time.passive = 
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config ,true)); // on scanne les APs dispo
    
    uint16_t apCount = 0; // nombre d'AP
    esp_wifi_scan_get_ap_num(&apCount); // on recupere le nombre d'AP
    ESP_LOGI(TAG, "Number of APs: %d", apCount);

    uint16_t max = 20;
    wifi_ap_record_t ap_list[20];
    esp_wifi_scan_get_ap_records(&max, ap_list);

    //esp_wifi_connect(); //se connecte

    
    for (int i = 0; i < 20; i++)
    {
        wifi_ap_record_t record = ap_list[i];
        //uint8_t name[33] = record.ssid;
        printf((const char *)record.ssid);
        printf("\n");
        //atoi((const char *)text

    }
    
    
}