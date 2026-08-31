
#include "cam_handler.h"
#include "esp_camera.h"
#include "esp_log.h"

static const char *TAG = "CAM_HDLR";

// ... (Keep your #define CAM_PINs and camera_config_t exactly as they are) ...

// =========================================================================
// Pin Configuration (Default layout for ESP-S3-EYE & Freenove ESP32-S3 CAM)
// =========================================================================
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4 // SDA
#define CAM_PIN_SIOC 5 // SCL
#define CAM_PIN_D7 16  // Y9
#define CAM_PIN_D6 17  // Y8
#define CAM_PIN_D5 18  // Y7
#define CAM_PIN_D4 12  // Y6
#define CAM_PIN_D3 10  // Y5
#define CAM_PIN_D2 8   // Y4
#define CAM_PIN_D1 9   // Y3
#define CAM_PIN_D0 11  // Y2
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
// =========================================================================

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,

    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // --- CRITICAL FIXES FOR OV3660 TIMING ---
    .xclk_freq_hz = 10000000, // Reduced to 10MHz for signal stability
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA, // 640x480
    .jpeg_quality = 12,          // Quality: 10-15 recommended
    .fb_count = 2,               // Double buffering
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY // Grab latest frame
};

void camera_handler_init() {
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera Init Failed 0x%x", err);
    return;
  }
  ESP_LOGI(TAG, "Camera Init Succeeded");
}

camera_fb_t *camera_acquire_frame() { return esp_camera_fb_get(); }

void camera_release_frame(camera_fb_t *fb) {
  if (fb) {
    esp_camera_fb_return(fb);
  }
}
