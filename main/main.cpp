#include<libs.h>

//#define CONFIG_WIFI_NAME WIFI_NAME
//#define CONFIG_WIFI_PW WIFI_PW

WIFIService::wifi_init(){
     auto ret = nvs_flash_init();
     if (ret != ESP_OK){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
     wifi_event_handler = xEventGroupCreate();

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP network stack");
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return ret;
    }

    ret = esp_wifi_set_default_wifi_sta_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default handlers");
        return ret;
    }

    tutorial_netif = esp_netif_create_default_wifi_sta();
    if (tutorial_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }
    
    auto config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
   
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID
			    	&wifi_event_cb, NULL, &wifi_event_handler));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID
			    	&ip_event_cb, NULL, &ip_event_handler));
}	


WIFIService::wifi_connect(char *wifi_ssid, char *wifi_pw){

}

WIFIService::wifi_deinit(){}

WIFIService::wifi_disconnect(){}

WIFIService::WIFIService(){
	wifi_init();
	wifi_connect();
}

WIFIService::~WIFIService(){
	wifi_deinit();
	wifi_disconnect();
}

extern "C" void app_main(void){
	WIFIService wifi_ser;
}
