
#include "web_server.h"
#include "cam_handler.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "WEB_SRV";

// 1. EMBEDDED FILE ACCESS
// These symbols point to your index.html file (added via CMakeLists.txt)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

// HTTP Streaming constants
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n";

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
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char part_buf[64];
  static int64_t last_frame = 0;

  if (!last_frame)
    last_frame = esp_timer_get_time();

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
    return res;

  while (true) {
    fb = camera_acquire_frame();
    if (!fb) {
      ESP_LOGE(TAG, "Camera capture failed");
      res = ESP_FAIL;
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;

      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY,
                                  strlen(_STREAM_BOUNDARY));

      if (res == ESP_OK) {
        size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
      }

      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
      }
    }

    if (fb) {
      camera_release_frame(fb);
      fb = NULL;
    }

    // res != ESP_OK means the browser closed the tab
    if (res != ESP_OK)
      break;

    // FPS calculation with safety check
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = (fr_end - last_frame) / 1000; // time in ms
    last_frame = fr_end;

    // Prevent division by zero if camera is faster than 1ms
    if (frame_time > 0) {
      float fps = 1000.0f / (float)frame_time;

      ESP_LOGI(TAG, "MJPG: %luKB %lums (%.1ffps)",
               (unsigned long)(_jpg_buf_len / 1024), (unsigned long)frame_time,
               fps);
    }
  }

  last_frame = 0;
  return res;
}

/*
 * SERVER INITIALIZATION
 */
void start_web_server(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Crucial for ESP32-CAM: Increase stack size for the server task
  config.stack_size = 8192;
  // Increase max URI handlers if you add many routes
  config.max_uri_handlers = 8;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t uri_get = {.uri = "/",
                           .method = HTTP_GET,
                           .handler = root_get_handler,
                           .user_ctx = NULL};

    httpd_uri_t uri_stream = {.uri = "/stream",
                              .method = HTTP_GET,
                              .handler = stream_handler,
                              .user_ctx = NULL};

    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_stream);

    ESP_LOGI(TAG, "Server started on port %d", config.server_port);
  } else {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }
}
