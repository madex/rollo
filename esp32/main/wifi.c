#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "wifi.h"

static const char *TAG = "wifi";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		// Immer wieder neu verbinden - das Geraet muss sich selbst heilen.
		ESP_LOGW(TAG, "Verbindung verloren, verbinde neu ...");
		vTaskDelay(pdMS_TO_TICKS(1000));
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
	}
}

void wifi_init_sta(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *netif = esp_netif_create_default_wifi_sta();
	esp_netif_set_hostname(netif, CONFIG_ROLLO_HOSTNAME);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
	                                                    &event_handler, NULL, NULL));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
	                                                    &event_handler, NULL, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ROLLO_WIFI_SSID,
			.password = CONFIG_ROLLO_WIFI_PASSWORD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	// Kein WLAN-Stromsparen: Web-UI soll sofort reagieren.
	esp_wifi_set_ps(WIFI_PS_NONE);

	ESP_LOGI(TAG, "WLAN gestartet, SSID: %s, Hostname: %s",
	         CONFIG_ROLLO_WIFI_SSID, CONFIG_ROLLO_HOSTNAME);
}
