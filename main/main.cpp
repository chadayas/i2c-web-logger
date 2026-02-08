#include<libs.h>

//#define CONFIG_WIFI_NAME WIFI_NAME
//#define CONFIG_WIFI_PW WIFI_PW
WIFIService::wifi_init()	




WIFIService::WIFIService(){
	wifi_init();
	wifi_connect();
}

WIFIService::~WIFIService(){
	wifi_deinit();
	wifi_disconnect();
}

extern "C" void app_main(void){
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();		
	nvs_flash_init();	
	auto res = esp_wifi_init(&config);
	std::cout << res << std::endl;
}
