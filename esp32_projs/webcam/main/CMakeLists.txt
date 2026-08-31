idf_component_register(SRCS "main.c"

                    "wifi_manager.c"

                    "cam_handler.c"

                    "web_server.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "index.html"
                    REQUIRES esp_http_server esp_netif nvs_flash esp_wifi esp_timer)
