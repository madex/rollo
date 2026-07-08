/*
 * Persistenz der Timer-Tabelle in NVS (Flash). Ersetzt die im Original
 * nie fertig implementierte Flash-Parameter-Block-Speicherung: Timer
 * ueberleben jetzt einen Stromausfall.
 */
#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include "rollo.h"
#include "storage.h"

static const char *TAG = "storage";

#define NVS_NAMESPACE "rollo"
#define NVS_KEY_TIMERS "timers"

void storage_load(void) {
	nvs_handle_t nvs;
	size_t size = sizeof(timeEvent_t) * NUM_TIMERS;
	esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
	if (err != ESP_OK) {
		ESP_LOGI(TAG, "keine gespeicherten Timer, nutze Defaults");
		return;
	}
	timeEvent_t loaded[NUM_TIMERS];
	err = nvs_get_blob(nvs, NVS_KEY_TIMERS, loaded, &size);
	if (err == ESP_OK && size == sizeof(loaded)) {
		memcpy(timeEvents, loaded, sizeof(loaded));
		ESP_LOGI(TAG, "Timer aus NVS geladen");
	} else {
		ESP_LOGI(TAG, "keine/inkompatible Timer in NVS (%s), nutze Defaults",
		         esp_err_to_name(err));
	}
	nvs_close(nvs);
}

void storage_save(void) {
	nvs_handle_t nvs;
	esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(err));
		return;
	}
	err = nvs_set_blob(nvs, NVS_KEY_TIMERS, timeEvents,
	                   sizeof(timeEvent_t) * NUM_TIMERS);
	if (err == ESP_OK)
		err = nvs_commit(nvs);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "Timer speichern fehlgeschlagen: %s", esp_err_to_name(err));
	else
		ESP_LOGI(TAG, "Timer in NVS gesichert");
	nvs_close(nvs);
}
