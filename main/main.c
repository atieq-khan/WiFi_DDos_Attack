#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "scan_wifi.h"
#include "webserver.h"

static const char TAG[] = "MAIN";

void app_main(void)
{
	ESP_LOGI(TAG, "in init (NVS)...");
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
   	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "Setting up everything......");
	wifi_task();
	ESP_LOGI(TAG, "before!!!!");
	HTTP_server_task();
	ESP_LOGI(TAG, "after!!!!!");
}

