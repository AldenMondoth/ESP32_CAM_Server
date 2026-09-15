#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cam_handler.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "web_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

static const char *TAG = "WEB_SRV";

httpd_handle_t server = NULL;
struct async_resp_arg {
  httpd_handle_t hd;
  int fd;
};

// 1. EMBEDDED FILE ACCESS
// These symbols point to your index.html file (added via CMakeLists.txt)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/*
 * HANDLER: Serves the main HTML page (/)
 */
static esp_err_t root_get_handler(httpd_req_t *req) {
  const size_t index_html_size = (index_html_end - index_html_start);
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

/*
 * HANDLER: Serves the MJPEG Video Stream (/stream)
 */
static esp_err_t stream_handler(httpd_req_t *req) {

  esp_err_t res = ESP_OK;
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "Handshake done, the new connection was opened");
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  uint8_t rx_buf[16];
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  ws_pkt.payload = rx_buf;

  res = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", res);
    return res;
  }

  if (ws_pkt.len) {

    ws_pkt.payload = rx_buf;
    res = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", res);
      return res;
    }
    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
  }

  ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);

  return ESP_OK;
}
void cam_stream_task(void *pvParameters) {
  httpd_handle_t server = (httpd_handle_t)pvParameters;
  static int64_t last_frame = 0;

  ESP_LOGI(TAG, "cam_stream_task started successfully! (server handle: %p)",
           server);

  if (!last_frame)
    last_frame = esp_timer_get_time();

  while (1) {
    // 1. MUST BE LOCAL TO THE LOOP: Resets array capacity to 8 on EVERY
    // iteration
    size_t fds = 8;
    int client_fds[8];
    int ws_client_count = 0;

    esp_err_t res = httpd_get_client_list(server, &fds, client_fds);

    if (res == ESP_OK && fds > 0) {
      for (size_t i = 0; i < fds; i++) {
        int client_info = httpd_ws_get_fd_info(server, client_fds[i]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
          ws_client_count++;
        }
      }
    }

    // 2. If no clients are connected, sleep and wait
    if (ws_client_count == 0) {
      vTaskDelay(pdMS_TO_TICKS(100)); // 100ms sleep
      continue;
    }

    // 3. Client is connected -> Capture frame
    camera_fb_t *fb = camera_acquire_frame();
    if (!fb) {
      ESP_LOGW(TAG, "Frame capture failed");
      vTaskDelay(pdMS_TO_TICKS(40));
      continue;
    }

    size_t jpg_len = fb->len;
    uint8_t *jpg_buf = fb->buf;

    httpd_ws_frame_t ws_img = {.final = true,
                               .fragmented = false,
                               .type = HTTPD_WS_TYPE_BINARY,
                               .payload = jpg_buf,
                               .len = jpg_len};

    // 4. Send to all active WebSocket clients
    for (size_t i = 0; i < fds; i++) {
      if (httpd_ws_get_fd_info(server, client_fds[i]) ==
          HTTPD_WS_CLIENT_WEBSOCKET) {
        esp_err_t send_err =
            httpd_ws_send_frame_async(server, client_fds[i], &ws_img);
        if (send_err != ESP_OK) {
          ESP_LOGW(TAG, "Failed to send frame to fd %d (err: %d)",
                   client_fds[i], send_err);
        }
      }
    }

    camera_release_frame(fb);
    fb = NULL;

    // 5. FPS Logging
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = (fr_end - last_frame) / 1000;
    last_frame = fr_end;

    if (frame_time > 0) {
      float fps = 1000.0f / (float)frame_time;
      ESP_LOGI(TAG, "MJPG: %luKB %lums (%.1ffps) [Clients: %d]",
               (unsigned long)(jpg_len / 1024), (unsigned long)frame_time, fps,
               ws_client_count);
    }

    vTaskDelay(pdMS_TO_TICKS(40)); // ~25 FPS
  }

  vTaskDelete(NULL);
}
/*
 * SERVER INITIALIZATION
 */
void start_web_server(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Crucial for ESP32-CAM: Increase stack size for the server task
  config.stack_size = 8192;
  // Increase max URI handlers if you add many routes
  config.max_uri_handlers = 8;
  config.send_wait_timeout = 1; // 1 second timeout instead of 6 seconds
  config.recv_wait_timeout = 1;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t uri_get = {.uri = "/",
                           .method = HTTP_GET,
                           .handler = root_get_handler,
                           .user_ctx = NULL};

    httpd_uri_t uri_stream = {.uri = "/stream",
                              .method = HTTP_GET,
                              .handler = stream_handler,
                              .user_ctx = NULL,
                              .is_websocket = true,
                              .handle_ws_control_frames = false,
                              .supported_subprotocol = NULL};

    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_stream);

    BaseType_t res = xTaskCreatePinnedToCore(cam_stream_task, "cam_stream_task",
                                             8192, server, 5, NULL, 1);

    if (res != pdPASS)
      ESP_LOGE(TAG, "Failed to create camera streaming task!");

    ESP_LOGI(TAG, "Server started on port %d", config.server_port);
  } else {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }
}
