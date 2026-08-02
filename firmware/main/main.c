#include "esp_log.h"
#include "bsp/esp-bsp.h"

#include "canbus.h"
#include "ui.h"

#define TAG "main"

void app_main(void)
{
  bsp_display_start();
  canbus_start();
  ui_init();

  ESP_LOGI(TAG, "mx5dash started");
}