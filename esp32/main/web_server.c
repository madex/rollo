/*
 * HTTP-Server (esp_http_server). Ersetzt uIP/lwIP-httpd des Originals.
 * "/"          -> eingebettete client.html (unveraenderte Web-UI)
 * "/ajax.cgi"  -> Kommandos ausfuehren + Status als JSON
 */
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

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

	ESP_LOGI(TAG, "HTTP-Server laeuft auf Port %d", config.server_port);
}
