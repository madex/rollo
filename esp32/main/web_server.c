/*
 * HTTP-Server (esp_http_server). Ersetzt uIP/lwIP-httpd des Originals.
 * "/"          -> eingebettete client.html (unveraenderte Web-UI)
 * "/ajax.cgi"  -> Kommandos ausfuehren + Status als JSON
 * "/ota"       -> POST: Firmware-Image in den inaktiven Slot, dann Reboot
 *                 curl -X POST --data-binary @build/rollo.bin http://<ip>/ota
 */
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rollo.h"
#include "httpd-uri-cmd.h"
#include "web_server.h"

static const char *TAG = "web";

extern const unsigned char client_html_start[] asm("_binary_client_html_start");
extern const unsigned char client_html_end[]   asm("_binary_client_html_end");

static char jsonBuf[8192]; // durch rollo_lock() geschuetzt

static esp_err_t root_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, (const char *)client_html_start,
	                       client_html_end - client_html_start);
}

static esp_err_t ajax_get_handler(httpd_req_t *req) {
	char *end;
	rollo_lock();
	httpd_uri_cmd(req->uri);
	end = genJson(jsonBuf, sizeof(jsonBuf) - 1);
	rollo_unlock();
	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, jsonBuf, end - jsonBuf);
}

static esp_err_t ota_post_handler(httpd_req_t *req) {
	const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);
	if (update == NULL) {
		return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
		                           "keine OTA-Partition");
	}
	if (req->content_len == 0 || req->content_len > update->size) {
		return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
		                           "Image fehlt oder zu gross");
	}
	ESP_LOGI(TAG, "OTA: %u Bytes -> %s", (unsigned)req->content_len,
	         update->label);

	esp_ota_handle_t ota;
	esp_err_t err = esp_ota_begin(update, OTA_WITH_SEQUENTIAL_WRITES, &ota);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
		return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
		                           "esp_ota_begin fehlgeschlagen");
	}

	static char buf[4096]; // eine OTA gleichzeitig, spart Handler-Stack
	int remaining = req->content_len;
	while (remaining > 0) {
		int len = httpd_req_recv(req, buf,
		                         remaining < (int)sizeof(buf) ? remaining
		                                                      : sizeof(buf));
		if (len == HTTPD_SOCK_ERR_TIMEOUT) {
			continue;
		}
		if (len <= 0) {
			esp_ota_abort(ota);
			return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
			                           "Empfang abgebrochen");
		}
		err = esp_ota_write(ota, buf, len);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_ota_write: %s", esp_err_to_name(err));
			esp_ota_abort(ota);
			return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
			                           "esp_ota_write fehlgeschlagen");
		}
		remaining -= len;
	}

	err = esp_ota_end(ota); // prueft Magic, Header und SHA256 des Images
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
		return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
		                           "Image ungueltig");
	}
	err = esp_ota_set_boot_partition(update);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_set_boot_partition: %s", esp_err_to_name(err));
		return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
		                           "Boot-Partition setzen fehlgeschlagen");
	}

	httpd_resp_set_type(req, "text/plain");
	httpd_resp_sendstr(req, "OK, starte neu...\n");
	ESP_LOGI(TAG, "OTA fertig, Reboot in 1 s");
	vTaskDelay(pdMS_TO_TICKS(1000)); // Antwort rausschieben lassen
	esp_restart();
	return ESP_OK; // nicht erreicht
}

void web_server_start(void) {
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size = 8192;
	config.lru_purge_enable = true;

	ESP_ERROR_CHECK(httpd_start(&server, &config));

	const httpd_uri_t root = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = root_get_handler,
	};
	httpd_register_uri_handler(server, &root);

	const httpd_uri_t ajax = {
		.uri = "/ajax.cgi",
		.method = HTTP_GET,
		.handler = ajax_get_handler,
	};
	httpd_register_uri_handler(server, &ajax);

	const httpd_uri_t ota = {
		.uri = "/ota",
		.method = HTTP_POST,
		.handler = ota_post_handler,
	};
	httpd_register_uri_handler(server, &ota);

	ESP_LOGI(TAG, "HTTP-Server laeuft auf Port %d", config.server_port);
}
