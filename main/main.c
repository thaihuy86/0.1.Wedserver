#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include <esp_log.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"


#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include <esp_http_server.h>

static const char *TAG = "wifi softAP";

#define EXAMPLE_ESP_WIFI_SSID      "ESP32 AP"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   0
#define EXAMPLE_MAX_STA_CONN       5

char html_index2[] =
		"</div>"
		"</form>"
		"<form>"
			"<div style ='padding-bottom: 10px; text-align: center;'>"
					"<label for='ssid'>Ten Wifi</label>"
					"<input style=' margin-left: 29px;' type='text' name='ssid' size='30'/><br>"
				"<br><label for='pass'>Mat Khau </label>"
				"<input style ='margin-left:10px' type='password' name='pass' size='30'/>"
			"</div>"

				"<div style='display: inline-block; margin-left: 45%;'>"
					"<a><button onclick= \"ketnoi()\"; style ='font-size: 20px;color: white;background-color: #e67e32; border-radius: 20px;text-align: center;'>Ket noi</button></a>"
		//								"<a href='/scan'><button onclick= \"scan()\"; style ='margin-left:10px;font-size: 20px;color: white;background-color: #e67e32; border-radius: 20px;text-align: center;'>Quet Wifi</button></a>"
				"</div>"
			"<script>"
			"function ketnoi() {"
			"alert('Da Luu SSID va Pass');"
			"}"
			"</script>"
			"<p id=\"demo\"></p>"
		"</form>"
		"<a><button onclick= \"scan()\"; style ='margin-left:44%;font-size: 20px;color: white;background-color: #e67e32; border-radius: 20px;text-align: center;'>Quet Wifi</button></a>"
		"<script>"
		"function scan() {"
		"alert('Dang Thuc Hien Quet WIFI');"
		"window.location.assign('http://192.168.4.1/scan');"
		"}"
		"</script>" "</body></html>";

typedef struct{
	uint8_t ssid[27];
	int8_t  rssi;
	uint8_t primary;
}listwifi_t;

//Store scan wifi
listwifi_t listWifi[8] = {0};
uint16_t ap_count = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
	        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
	        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
	                 MAC2STR(event->mac), event->aid);
	    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
	        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
	        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
	                 MAC2STR(event->mac), event->aid);
	    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_scan(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    uint16_t number = 16;
    wifi_ap_record_t ap_info[16] = {0};

    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_records(&number, ap_info);
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    for (int i = 0; i < ap_count; i++)
    {
        memcpy(listWifi[i].ssid, ap_info[i].ssid, strlen(&ap_info[i].ssid));
        listWifi[i].rssi = ap_info[i].rssi;
        listWifi[i].primary = ap_info[i].primary;
    }
}

/////////////
//WED
////////////
static esp_err_t wed_get_handler(httpd_req_t *req)
{
	wifi_scan();
	char html[2048]={0};
	char c[1024]={0};
	char http_index_hml[3*1024] =
			"<!DOCTYPE html>"
			"<html>"
			"<head>"
			"<title>Test</title>"
			"<style>"
			"h2{padding: 20px;text-transform: uppercase;"
			"letter-spacing: 1px;color: #e67e32;"
			"font-size: 40px;"
			"text-align: center;}"
			"</style>"
			"</head>"
			"<body>"
				"<h2>Cau Hinh Wifi</h2>"
			"<form>"
			"<label style='color: #e67e32; font-size: 20px; margin-left:45%;'>Danh Sach Wifi</label>"
			"<div style ='padding-bottom: 10px; font-size: 10px; margin-left:45%;'>"
			;
	for (int i = 0; i < ap_count; i++)
	{
		char buffer[128];
		sprintf(buffer, "<label> %s  %d  %d </label><br>", listWifi[i].ssid, listWifi[i].rssi, listWifi[i].primary);
		strcat(html,buffer);
	}
	strcat(http_index_hml,html);
	strcat(http_index_hml,html_index2);
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    httpd_resp_send(req,http_index_hml, strlen(http_index_hml));

    ESP_LOGI(TAG, "%s", c);
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    return ESP_OK;
}

static const httpd_uri_t wed = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = wed_get_handler,
};

/////////////
//SCAN_WIFI
////////////
static esp_err_t scan_get_handler(httpd_req_t *req)
{
	wifi_scan();
	char http_index_hml[3*1024] =
				"<!DOCTYPE html>"
				"<html>"
				"<head>"
				"<title>Test</title>"
				"<style>"
				"h2{padding: 20px;text-transform: uppercase;"
				"letter-spacing: 1px;color: #e67e32;"
				"font-size: 40px;"
				"text-align: center;}"
				"</style>"
				"</head>"
				"<body>"
					"<h2>Cau Hinh Wifi</h2>"
				"<form>"
				"<div style ='padding-bottom: 10px; font-size: 30px; margin-left:45%;'>"
				"<label style='color: #e67e32;'>Danh Sach Wifi</label>"
				;
	char html[2048]={0};
	for (int i = 0; i < ap_count; i++)
	{
		char buffer[128];
		sprintf(buffer, "<label> %s  %d  %d </label><br>", listWifi[i].ssid, listWifi[i].rssi, listWifi[i].primary);
		strcat(html,buffer);
	}
	strcat(http_index_hml,html);
	strcat(http_index_hml,html_index2);
    /* Send response with custom headers and body set as the
     * string passed in user context*/
	httpd_resp_send(req,http_index_hml, strlen(http_index_hml));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    return ESP_OK;
}

static const httpd_uri_t scan = {
    .uri       = "/scan",
    .method    = HTTP_GET,
    .handler   = scan_get_handler,
};



static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &scan);
        httpd_register_uri_handler(server, &wed);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void app_main(void)
{
	static httpd_handle_t server = NULL;
	//Initialize NVS
	    esp_err_t ret = nvs_flash_init();
	    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	      ESP_ERROR_CHECK(nvs_flash_erase());
	      ret = nvs_flash_init();
	    }
	    ESP_ERROR_CHECK(ret);
	    wifi_init_softap();
	    server = start_webserver();
}

