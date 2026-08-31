
#ifndef CAM_HANDLER_H
#define CAM_HANDLER_H

#include "esp_camera.h"

static camera_config_t camera_config;

void camera_handler_init();
camera_fb_t *camera_acquire_frame();
void camera_release_frame(camera_fb_t *fb);

#endif
