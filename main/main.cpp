#include<libs.h>
#include<fstream>

struct ws_resp_arg {
	httpd_handle_t hd;
	int fd;
	char data[64];
};


static void wifi_event_cb(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    WifiService *svc = (WifiService *)arg;

    if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, connecting...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (svc->wifi_retry_count < svc->WIFI_RETRY_ATTEMPT) {
            ESP_LOGW(TAG, "Disconnected, retrying (%d/%d)", svc->wifi_retry_count + 1, svc->WIFI_RETRY_ATTEMPT);
            esp_wifi_connect();
            svc->wifi_retry_count++;
        } else {
            ESP_LOGE(TAG, "Failed to connect after %d attempts", svc->WIFI_RETRY_ATTEMPT);
            xEventGroupSetBits(svc->wifi_event_group, WIFI_FAIL_BIT);
        }
    }
}

static void ip_event_cb(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
    WifiService *svc = (WifiService *)arg;

    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        svc->wifi_retry_count = 0;
        xEventGroupSetBits(svc->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t WifiService::init(){
     auto ret = nvs_flash_init();
     if (ret != ESP_OK){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
     wifi_event_group = xEventGroupCreate();

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
    
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
			    	&wifi_event_cb, this, &wifi_event_handler));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID,
			    	&ip_event_cb, this, &ip_event_handler));
    return ret;
}	


esp_err_t WifiService::connect(){
    wifi_config_t wifi_config{};

    strncpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_NAME, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, CONFIG_WIFI_PW, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // default is WIFI_PS_MIN_MODEM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); // default is WIFI_STORAGE_FLASH

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "Connecting to Wi-Fi network: %s",wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to Wi-Fi network: %s", wifi_config.sta.ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi network: %s", wifi_config.sta.ssid);
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Unexpected Wi-Fi error");
    return ESP_FAIL;
}

esp_err_t WifiService::deinit(){
    esp_err_t ret = esp_wifi_stop();
    if (ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGE(TAG, "Wi-Fi stack not initialized");
        return ret;
    }

    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(tutorial_netif));
    esp_netif_destroy(tutorial_netif);

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler));

    return ESP_OK;
}

esp_err_t WifiService::disconnect(){
    if (wifi_event_group) {
        vEventGroupDelete(wifi_event_group);
    }

    return esp_wifi_disconnect();
}

WifiService::WifiService(){
	init();
	connect();
}

WifiService::~WifiService(){
	deinit();
	disconnect();
}

adc_oneshot_unit_handle_t adc1_handle;

bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle){
	adc_cali_handle_t handle = NULL;
    	esp_err_t ret = ESP_FAIL;
    	bool calibrated = false;
	auto calib_tag = "[CALIBRATION]";	
	if (!calibrated) {
		ESP_LOGI(calib_tag, "calibration scheme version is %s", "Line Fitting");
		
		adc_cali_line_fitting_config_t cali_config{};
		cali_config.unit_id = unit;
		cali_config.atten = atten;
		cali_config.bitwidth = ADC_BITWIDTH_DEFAULT; 

		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(calib_tag, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(calib_tag, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(calib_tag, "Invalid arg or no memory");
	}

	return calibrated;
} 


void Handlers::ws_async_send(void* arg){
		auto *resp_arg = (ws_resp_arg *)arg;
		httpd_ws_frame_t ws_frame{};

		ws_frame.payload = (uint8_t*)resp_arg->data;
		ws_frame.len = strlen(resp_arg->data);
		ws_frame.type = HTTPD_WS_TYPE_TEXT;

		httpd_ws_send_frame_async(resp_arg->hd, resp_arg->fd, &ws_frame);
		delete resp_arg;
}


esp_err_t Handlers::root(httpd_req_t *req) {
    std::ifstream file("/spiffs/index.html");
   	auto name = "[HTML]";
	ESP_LOGI(name, "html sent");
    if (!file.is_open()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open HTML file");
        return ESP_FAIL;
    }
    std::string html((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html.c_str(), html.length());
}

esp_err_t Handlers::css(httpd_req_t *req) {
    std::ifstream file("/spiffs/style.css");
    auto name= "[CSS]";
	ESP_LOGI(name, "css sent");
    if (!file.is_open()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open css file");
        return ESP_FAIL;
    }
    std::string css((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    httpd_resp_set_type(req, "text/css");
    return httpd_resp_send(req, css.c_str(), css.length());
}


esp_err_t Handlers::js(httpd_req_t *req) {
    std::ifstream file("/spiffs/graph.js");
    auto name = "[JavaScript]";
	ESP_LOGI(name, "javascript sent");
    if (!file.is_open()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open javascript file");
        return ESP_FAIL;
    }
    std::string js((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    httpd_resp_set_type(req, "text/javascript");
    return httpd_resp_send(req, js.c_str(), js.length());
}

esp_err_t Handlers::websock(httpd_req_t *req){
   	auto ws_tag = "[WEB SOCKET]"; 
	if (req->method == HTTP_GET) {
        ESP_LOGI(ws_tag, "WebSocket client connected");
        return ESP_OK;
    }
	httpd_ws_frame_t pkt{};
	pkt.type = HTTPD_WS_TYPE_TEXT;
	esp_err_t intial_read = httpd_ws_recv_frame(req, &pkt, 0);
	
	if(pkt.len){
		uint8_t *buf = new uint8_t[pkt.len + 1]();
		pkt.payload = buf;
		httpd_ws_recv_frame(req, &pkt, pkt.len);
		delete[] buf;	
	}
   	
	// read sensor and send to client
    int adc_raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
    float voltage = adc_raw * (3.3f / 4095.0f);
    float bac = voltage / 33.0f;

    auto *resp_arg = new ws_resp_arg;
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    snprintf(resp_arg->data, sizeof(resp_arg->data), "{\"bac\": %.4f, \"raw\": %d, \"mv\": %.0f}", bac, adc_raw, voltage * 1000);

    return httpd_queue_work(req->handle, Handlers::ws_async_send, resp_arg);
}

httpd_handle_t Httpserver::init(){
	if (httpd_start(&svr, &cfg) == ESP_OK){
		ESP_LOGI(TAG, "HTTP server started");

		httpd_uri_t root_s{};
		root_s.uri = "/";
		root_s.method = HTTP_GET;
		root_s.handler = Handlers::root;

		httpd_uri_t css_s{};
		css_s.uri = "/style.css";
		css_s.method = HTTP_GET;
		css_s.handler = Handlers::css;

		httpd_uri_t js_s{};
		js_s.uri = "/graph.js";
		js_s.method = HTTP_GET;
		js_s.handler = Handlers::js;

		httpd_uri_t ws_s{};
		ws_s.uri = "/ws";
		ws_s.method = HTTP_GET;
		ws_s.handler = Handlers::websock;
		ws_s.is_websocket = true;

		register_route(&root_s);
		register_route(&css_s);
		register_route(&js_s);
		register_route(&ws_s);

		return svr;
	} else{
		ESP_LOGE(TAG, "Unable to start server");
		return NULL;
	}
}

void Httpserver::deinit(){
	if(svr != NULL)
		httpd_stop(svr);
}

esp_err_t Httpserver::register_route(const httpd_uri_t *uri_cfg){
    return httpd_register_uri_handler(svr, uri_cfg);
}

Httpserver::Httpserver(){
	init();
}

Httpserver::~Httpserver(){
	deinit();
}

extern "C" void app_main(void){
	WifiService wifi; // connects to the wifi.
	
	size_t total{}, used{};

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t cfg{};
	cfg.base_path = "/spiffs";
	cfg.partition_label = NULL;
	cfg.max_files = 5;
	cfg.format_if_mount_failed = true;
 
	esp_err_t ret = esp_vfs_spiffs_register(&cfg);
	
	if (ret != ESP_OK) {
	    if (ret == ESP_FAIL) {
        	ESP_LOGE(TAG, "Failed to mount or format filesystem");
    	  } else if (ret == ESP_ERR_NOT_FOUND) {
        	ESP_LOGE(TAG, "Failed to find SPIFFS partition");
          } else {
   	     	ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    	}
    	  return;
    	}
	
     ret = esp_spiffs_info(cfg.partition_label, &total, &used);
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
         esp_spiffs_format(cfg.partition_label);
         return;
     } else {
         ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
     }

	adc_oneshot_unit_init_cfg_t init_cfg{};
	init_cfg.unit_id = ADC_UNIT_1;
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

	adc_oneshot_chan_cfg_t adc_cfg{};
	adc_cfg.atten = ADC_ATTEN_DB_12;
	adc_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &adc_cfg));

    	adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    	
	adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);

     	Httpserver server;
	     
     while(true){
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
