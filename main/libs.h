#pragma once

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include<esp_http_server.h>
#include<esp_wifi.h>
#include<esp_event.h>
#include<esp_netif.h>
#include<nvs_flash.h>
#include<iostream>

class WIFIService{
	public:
		WIFIService();
		~WIFIService();
	private:
		esp_err_t wifi_init();
		esp_err_t wifi_connect(char * wifi_ssid, char * wifi_pw);
		esp_err_t wifi_deinit();
		esp_err_t wifi_disconnect();
};			
