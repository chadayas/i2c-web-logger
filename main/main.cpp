#include<libs.h>


void start(){
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t server = NULL;

	if (httpd_start(&server, &config) == ESP_OK){
		std::cout << "Server created" << std::endl;
		return;
	} else{
		auto res = httpd_start(&server, &config);
		std::cout << res << std::endl;
   } 
	
}

extern "C" void app_main(void){
	
	start();	
}
