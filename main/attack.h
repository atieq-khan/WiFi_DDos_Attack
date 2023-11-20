/*
 * attack.h
 *
 *  Created on: Oct 22, 2023
 *      Author: Atieq
 */

#ifndef MAIN_ATTACK_H_
#define MAIN_ATTACK_H_


#define WIFI_ATTACK_STACK_SIZE 		3072
#define WIFI_ATTACK_PRIORITY		5
#define WIFI_ATTACK_CORE_ID         1

typedef enum
{
    WIFI_ATTACK_START = 0,
	WIFI_ATTACK_STOP,
    WIFI_SCAN_RESTART,
    WIFI_ATTACK_TASK_STOP,
}wifi_attack_message_e;

typedef struct
{
    wifi_attack_message_e msgID;
}wifi_attack_message_t;

BaseType_t wifi_attack_send_massage(wifi_attack_message_e msgID);

void wifi_attack();

void wifi_init_promiscuous();

void send_Deauth(uint8_t *bssid);

#endif /* MAIN_ATTACK_H_ */
