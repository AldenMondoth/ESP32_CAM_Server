#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"

static esp_err_t stream_handler(httpd_req_t *req);

void start_web_server(void);

#endif
