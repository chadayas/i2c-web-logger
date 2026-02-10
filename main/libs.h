#pragma once

#include<stdio.h>
#include<inttypes.h>
#include<string>
#include<iostream>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define TAG "wifi"

#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PSK

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

class WIFIService{
	public:
		WIFIService();
		~WIFIService();
	public:
		esp_err_t init();
		esp_err_t connect();
		esp_err_t deinit();
		esp_err_t disconnect();
	public:
		const int WIFI_RETRY_ATTEMPT = 3;
		int wifi_retry_count = 0;

		esp_netif_t *tutorial_netif = NULL;
		esp_event_handler_instance_t ip_event_handler;
		esp_event_handler_instance_t wifi_event_handler;

		EventGroupHandle_t wifi_event_group = NULL;
};			
