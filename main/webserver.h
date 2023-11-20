
#ifndef MAIN_WEBSERVER_H_
#define MAIN_WEBSERVER_H_

// HTTP Server Task
#define HTTP_SERVER_TASK_STACK_SIZE 		9216
#define HTTP_SERVER_TASK_PRIORTITY			4
#define HTTP_SERVER_TASK_CORE_ID			1

typedef enum
{
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIIF_CONNECT_SUCCESSS,
	HTTP_MSG_ATTACK,
    HTTP_MSG_WIFI_START,
    WIFI_START_HTTP_SERVER,
    HTTP_MSG_WIFI_REFREASH,
    HTTP_MSG_ATTACK_STOP,
}http_server_message_e;

typedef struct
{
    http_server_message_e msgID;
}http_server_message_t;

extern QueueHandle_t http_server_Queue;
extern uint8_t bssid_attack[6];
extern int STA_CHANNEL;

/**
 * Sends a massage to the queue
 * @param msgID massage ID from the http_server_massge_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 */

BaseType_t http_server_monitor_send_masssge(http_server_message_e msgID);

void web_server_start();

void HTTP_server_task();

//void web_server_stop();

#endif /* MAIN_SCAN_WIFI_H_ */