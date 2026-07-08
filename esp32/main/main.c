/*
 * Rollocontrol auf ESP32-C3 (Port der Stellaris-LM3S9B96-Version).
 * WLAN statt Ethernet, NTP statt manuell gestellter Uhr, Timer in NVS.
 */
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

#include "rollo.h"
#include "storage.h"
#include "wifi.h"
#include "web_server.h"

static const char *TAG = "main";

void app_main(void) {
	// NVS (fuer WLAN-Kalibrierdaten und Timer-Speicherung)
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	ESP_LOGI(TAG, "Rollocontrol v0.6 ESP32 (Martin Ongsiek)");

	rollo_init();
	storage_load();
	wifi_init_sta();

	// Zeitzone (Sommer-/Winterzeit automatisch) und NTP
	setenv("TZ", CONFIG_ROLLO_TZ, 1);
	tzset();
	esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_ROLLO_NTP_SERVER);
	ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp_config));

	web_server_start();

	// 10-ms-Regelschleife (entspricht der alten while(1)/ticks-Schleife)
	TickType_t lastWake = xTaskGetTickCount();
	int msCount = 0;
	while (1) {
		vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
		rollo_lock();
		msCount += 10;
		if (msCount >= 1000) {
			msCount = 0;
			rollo_secondTick();
		}
		rollo_Cont();
		rollo_unlock();
	}
}
