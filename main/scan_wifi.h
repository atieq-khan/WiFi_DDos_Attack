/*
 * scan_wifi.h
 *
 *  Created on: Oct 21, 2023
 *      Author: Atieq
 */
#include "esp_wifi.h"
#include "esp_netif.h"

#ifndef MAIN_SCAN_WIFI_H_
#define MAIN_SCAN_WIFI_H_

#define MAX_LIST                    10
#define WIFI_SCAN_STACK_SIZE 	    4096
#define WIFI_SCAN_PRIORITY			4
#define	WIFI_AP_SSID				"ESP-32 AP"  // AP name
#define WIFI_AP_PASSWORD			"12345678"  // AP password
#define WIFI_AP_CHANNEL				1		    // AP Channel
#define WIFI_AP_SSID_HIDDEN			0			// AP Visibility
#define WIFI_AP_MAX_CONNECTIONS		5			// AP max connections
#define WIFI_AP_BEACON_INTERVAL		100			// AP beacon (100ms)
#define WIFI_AP_IP					"192.168.0.1"	// AP default IP
#define WIFI_AP_GATEWAY				"192.168.0.1"	// Default gateway
#define WIFI_AP_NETMASK				"255.255.255.0" // Default net mask
#define WIFI_AP_BANWIDTH			WIFI_BW_HT20	// AP bandwidth 20 MHz
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE	// Disable power saving
#define WIFI_SSID_LENGTH			22				// IEEE Standard length
#define WIFI_PASSWORD_LENGTH		64				// IEEE Standard length
#define MAX_CONNECTION_RETRIES		5				// Retry number on disconnect.


extern wifi_ap_record_t ap_record[MAX_LIST];
extern wifi_config_t ap_config;


/**
 * This is the Message queue for WiFi.
 */
typedef enum{   
    WIFI_MSG_START,             // For starting the wifi.
    WIFI_MSG_SCAN_START,        // For start the scan (in STA mode)
    WIFI_MSG_DISPLAY,           // Displaying the found wifi.
    WIFI_MSG_ATTACK,            // Switch for attack mode.
    WIFI_MSG_HTTP_START,
}wifi_app_massage_e;

typedef struct {
    wifi_app_massage_e msgID;
} wifi_app_queue_massage_t;

/**
 * Sends a massage to the queue
 * @param msgID massage ID from the wifi_app_massge_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 */
BaseType_t wifi_app_send_massage(wifi_app_massage_e msgID);

/**
 * 
 */
void wifi_task();

void ap_list_get();

void wifi_task_scan();

const char* auth_wifi(wifi_auth_mode_t auth);
#endif /* MAIN_SCAN_WIFI_H_ */
