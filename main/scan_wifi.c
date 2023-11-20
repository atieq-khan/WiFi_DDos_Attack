/*
 * sacn_wifi.c
 *
 *  Created on: Oct 21, 2023
 *      Author: Atieq
 */

#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/queue.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "scan_wifi.h"
#include "attack.h"
#include "webserver.h"

static const char TAG[] = "WIFI";

wifi_ap_record_t ap_record[MAX_LIST];

static QueueHandle_t wifiQueue;

TaskHandle_t wifitaskHandle = NULL;

esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap  = NULL;

/**
 * check the secrity of wifi
 * @param is wifi_auth_mode_t auth
 * @return char*
 */
const char* auth_wifi(wifi_auth_mode_t auth)
{
	switch(auth)
	{
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
    default: return "UNKNOWN";
	}
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } 
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
	if(event_id == WIFI_EVENT_SCAN_DONE)
	{
		ESP_LOGI("WIFI", "The sacn has done!!!!");
		wifi_app_send_massage(WIFI_MSG_HTTP_START);
	}
}


static void wifi_init()
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

	wifi_config_t ap_config ={
		.ap = {
			.ssid = WIFI_AP_SSID,
			.ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASSWORD,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
	
	wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = "WIFI_SSID",
            .password = "WIFI_PASS",
        },
    };

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);


	uint8_t primary_channel;
    // Get the current AP channel
    esp_wifi_get_channel(&primary_channel, NULL);
    // Display the AP channel information
    ESP_LOGI("ATTACK", "Current AP Channel: %d", primary_channel);

	//http_server_monitor_send_masssge(HTTP_MSG_WIFI_CONNECT_INIT);

	wifi_app_send_massage(WIFI_MSG_SCAN_START);
}

void wifi_display(uint16_t ap_num)
{
	uint8_t bssid[6] = {0};
	ESP_LOGI(TAG, "Displaying WiFi lists....");
	printf("Number of Access Points Found: %d\n", ap_num);
	printf("\n");
	printf("               SSID              | Channel | RSSI |   Authentication Mode  | BSSID |\n");
	printf("***************************************************************\n");
	for (int i = 0; i < ap_num; i++)
	{
		memcpy(bssid, ap_record[i].bssid, 6);

		printf("%32s | %7d | %4d | %22s | %02x:%02x:%02x:%02x:%02x:%02x |\n", 
		(char *)ap_record[i].ssid, ap_record[i].primary,ap_record[i].rssi, auth_wifi(ap_record[i].authmode), 
		bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	}
	printf("***************************************************************\n");
}

void ap_list_get()
{
	ESP_LOGI(TAG, "in ap_list_get...");
	uint16_t ap_num = MAX_LIST;
	wifi_scan_config_t scan_config = {
			.ssid = 0,
			.bssid = 0,
			.channel = 0,
			.show_hidden = 0,
	};
	
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_record));
	ESP_ERROR_CHECK(esp_wifi_scan_stop());
	wifi_display(ap_num);
	http_server_monitor_send_masssge(HTTP_MSG_WIFI_REFREASH);
}

void wifi_task_scan()
{
   wifi_app_queue_massage_t receivedMessage;
    
    while (true) {
        if (xQueueReceive(wifiQueue, &receivedMessage, portMAX_DELAY)) {
            switch(receivedMessage.msgID) {
                case WIFI_MSG_START:
                    ESP_LOGI(TAG, "Received START message. Initializing WiFi...");
        			wifi_init();
        			break;
                case WIFI_MSG_SCAN_START:
                    ap_list_get();
                    break;
                case WIFI_MSG_ATTACK:
                    ESP_LOGI(TAG, "starting attack.....");
					http_server_monitor_send_masssge(HTTP_MSG_ATTACK);
					wifi_attack();					
                    break;
				case WIFI_MSG_HTTP_START:
					// send msg to http_server.
					http_server_monitor_send_masssge(HTTP_MSG_WIFI_START);
					break;
				//case WIFI_SEND_DATA_TO_HTTP:
					
                default:
                    break;
				}
			}
		ESP_LOGI("loop", "this is running.....");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

BaseType_t wifi_app_send_massage(wifi_app_massage_e msgID)
{
	wifi_app_queue_massage_t msg;
	msg.msgID = msgID;
	return xQueueSend(wifiQueue, &msg, portMAX_DELAY);
}

void wifi_task()
{
	wifiQueue = xQueueCreate(9, sizeof(wifiQueue));

	xTaskCreatePinnedToCore(
			wifi_task_scan,         // Task function
	        "WiFiScanTask",         // Name of the task (for debugging purposes)
			WIFI_SCAN_STACK_SIZE,   // Stack size (in words, not bytes!)
	        NULL,                   // Task parameter
			4,     // Priority (higher number means higher priority)
	        wifitaskHandle,                    // Task handle (not needed here, so set to NULL)
			0
	    );
	
	wifi_app_send_massage(WIFI_MSG_START);
}

