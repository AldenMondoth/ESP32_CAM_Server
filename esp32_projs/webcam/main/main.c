
#include "cam_handler.h"
#include "web_server.h"
#include "wifi_manager.h"

void app_main(void) {

  wifi_manager_init();

  camera_handler_init();

  start_web_server();
}
