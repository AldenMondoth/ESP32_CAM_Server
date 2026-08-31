#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_event.h"

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

void wifi_manager_init();

#endif
