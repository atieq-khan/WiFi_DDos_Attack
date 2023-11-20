#include "string.h"
#include "stdio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "freertos/queue.h"

#include "scan_wifi.h"
#include "attack.h"
#include "webserver.h"

uint8_t bssid_attack[6];
int STA_CHANNEL;

TaskHandle_t webtaskHandle = NULL;

extern const uint8_t index_html_start[]  	asm("_binary_index_html_start");
extern const uint8_t index_html_end[] 		asm("_binary_index_html_end");

// Tag used for ESP serial console massage.
static const char TAG[] = "http_server";
httpd_config_t config;

// HTTP web server queue
QueueHandle_t http_server_Queue;

// web server handler
httpd_handle_t server_handler = NULL;


esp_err_t receive_data_handler(httpd_req_t *req) {

	char content[100]; // Adjust buffer size based on expected data
	int channel = 0;
    int received = httpd_req_recv(req, content, sizeof(content));

	printf("the request: %s", content);

	if (received <= 0) {
        // Handle error
		ESP_LOGW(TAG, "Not getting the data!!!!");
        return -1;
    }
    // Null-terminate the received data
    content[received] = '\0';

    // Parse the received data
    char *bssid_ptr = strstr(content, "bssid=");
    char *channel_ptr = strstr(content, "channel=");
	char *flag_ptr = strstr(content, "stop_flag=");
	if ((!bssid_ptr || !channel_ptr) && !flag_ptr) {
        // Malformed request
		ESP_LOGW(TAG, "Request was Malformed");
        return -1;
    }

	if(flag_ptr)
	{
		ESP_LOGI("attack","checking.....................................\n");
		flag_ptr = NULL;
		wifi_attack_send_massage(WIFI_ATTACK_STOP);
		httpd_resp_set_type(req, "text/plain");
    	httpd_resp_send(req, "Stop attack request received", -1);
		ESP_LOGI("attack","checking recvied.....................................\n");


		return ESP_OK;
	}

	sscanf(bssid_ptr, "bssid=%2hhx%%3A%2hhx%%3A%2hhx%%3A%2hhx%%3A%2hhx%%3A%2hhx",
    	&bssid_attack[0], &bssid_attack[1], &bssid_attack[2],
    	&bssid_attack[3], &bssid_attack[4], &bssid_attack[5]);

	sscanf(channel_ptr, "channel=%d", &channel);

	STA_CHANNEL = channel;
    printf("\n%02x:%02x:%02x:%02x:%02x:%02x | %d\n", bssid_attack[0], bssid_attack[1], bssid_attack[2], bssid_attack[3], bssid_attack[4], bssid_attack[5], STA_CHANNEL);

	

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "recveed the data", -1);

	ESP_LOGI("My tag", "in attacking mode");
	wifi_app_send_massage(WIFI_MSG_ATTACK);

    return ESP_OK;

}

httpd_uri_t uri_post_data = {
    .uri = "/received",
    .method = HTTP_POST,
    .handler = receive_data_handler,
	.user_ctx = NULL
};

esp_err_t trigger_scan_handler(httpd_req_t *req)
{
    // Send message to start Wi-Fi scan
    wifi_app_send_massage(WIFI_MSG_SCAN_START);

    // Send a success response
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Scan triggered", -1);

    return ESP_OK;
}

httpd_uri_t trigger_scan = {
    .uri = "/trigger-scan",
    .method  = HTTP_POST,
    .handler = trigger_scan_handler,
    .user_ctx = NULL
};

esp_err_t index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "requested the HMTL pages");
	httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)index_html_start, index_html_end - index_html_start);

    return ESP_OK;
}

esp_err_t display_handler(httpd_req_t *req)
{
    char record[256] = {0};
	size_t response_size = 4 * 256;
    char *response = (char *)malloc(response_size);
    uint8_t bssid[6] = {0};

	memset(response, 0, response_size);
    
    for (int i = 0; i < 4; i++)
    {
        memcpy(bssid, ap_record[i].bssid, 6);
        
        snprintf(record, sizeof(record), 
                 "<p><br/>%s | %d | %d | %22s | %02x:%02x:%02x:%02x:%02x:%02x |<br/></p>", 
                 (char *)ap_record[i].ssid, ap_record[i].primary, ap_record[i].rssi, 
                 auth_wifi(ap_record[i].authmode), 
                 bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

		strncat(response, record, response_size - strlen(response) - 1);
    }
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, response, strlen(response));

    free(response);

    return ESP_OK;
}


httpd_uri_t display_data = {
	.uri = "/display",
	.method  = HTTP_GET,
	.handler = display_handler,
	.user_ctx = NULL
};


httpd_handle_t http_server_configure()
{
	httpd_config_t temp_config = HTTPD_DEFAULT_CONFIG();
	memcpy(&config,&temp_config,sizeof(httpd_config_t));

	ESP_LOGI(TAG,
			"http_server_configure: Starting server on port: %d with task priority: %d ",
			config.server_port,
			config.task_priority);

	if (httpd_start(&server_handler, &config) == ESP_OK) {

		httpd_uri_t index_html = {
			.uri       = "/",
			.method    = HTTP_GET,
			.handler   = index_html_handler,
			.user_ctx = NULL
		};
        httpd_register_uri_handler(server_handler, &index_html);
		httpd_register_uri_handler(server_handler, &uri_post_data);

		return server_handler;

    }
	return NULL;
}


void web_server_start()
{
	ESP_LOGI(TAG, "Server!!!!!");
    if(server_handler == NULL)
	{
	    server_handler = http_server_configure();
	}

}

static void http_server_queue_handler_task()
{

	web_server_start();

	http_server_message_t receivedMessage;
    
    while (true) {
        if (xQueueReceive(http_server_Queue, &receivedMessage, portMAX_DELAY)) {
            switch(receivedMessage.msgID) {

				case HTTP_MSG_WIFI_START:
				httpd_register_uri_handler(server_handler, &display_data);
				break;

				case HTTP_MSG_WIFI_REFREASH:
				httpd_register_uri_handler(server_handler, &trigger_scan);
				break;

				case HTTP_MSG_ATTACK:
				httpd_register_uri_handler(server_handler, &uri_post_data);
				break;

				// case HTTP_MSG_ATTACK_STOP:
				// httpd_register_uri_handler(server_handler, &stopping_attack);
				// break;

                default:
                    break;
				}
			}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

BaseType_t http_server_monitor_send_masssge(http_server_message_e msgID)
{
	http_server_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_Queue, &msg, portMAX_DELAY);

}


void HTTP_server_task()
{
	http_server_Queue = xQueueCreate(5, sizeof(QueueHandle_t));
	xTaskCreatePinnedToCore(
			http_server_queue_handler_task,         		// Task function
	        "HTTP_server_task",         	// Name of the task (for debugging purposes)
			HTTP_SERVER_TASK_STACK_SIZE,   	// Stack size (in words, not bytes!)
	        NULL,                   		// Task parameter
			HTTP_SERVER_TASK_PRIORTITY,     // Priority (higher number means higher priority)
	        webtaskHandle,                    		// Task handle (not needed here, so set to NULL)
			HTTP_SERVER_TASK_CORE_ID		//
	    );
}