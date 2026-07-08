idf_component_register(SRCS "main.c"
                            "rollo.c"
                            "httpd-uri-cmd.c"
                            "web_server.c"
                            "wifi.c"
                            "storage.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "client.html"
                       PRIV_REQUIRES nvs_flash esp_wifi esp_netif esp_http_server esp_timer driver lwip)
