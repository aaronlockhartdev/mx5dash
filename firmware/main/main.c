#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#include "canbus.h"

#undef BSP_LCD_COLOR_FORMAT
#define BSP_LCD_COLOR_FORMAT (ESP_LCD_COLOR_FORMAT_RGB888)
#undef BSP_LCD_BITS_PER_PIXEL
#define BSP_LCD_BITS_PER_PIXEL (24)

static lv_obj_t *rpm_arc;

static void update_gauge(lv_timer_t *timer) {
  (void)timer;
  canbus_data_t *data = canbus_get_latest();
  bsp_display_lock(-1);
  lv_arc_set_value(rpm_arc, data->rpm);
  bsp_display_unlock();
}

void app_main(void)
{
  bsp_display_start();
  canbus_start();

  bsp_display_lock(-1);

  rpm_arc = lv_arc_create(lv_screen_active());
  lv_arc_set_min_value(rpm_arc, 0);
  lv_arc_set_max_value(rpm_arc, 9000);
  lv_arc_set_bg_angles(rpm_arc, 0, 270);
  lv_arc_set_rotation(rpm_arc, 135);
  lv_arc_set_value(rpm_arc, 0);
  lv_obj_remove_style(rpm_arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(rpm_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(rpm_arc, lv_pct(85), lv_pct(85));
  lv_obj_center(rpm_arc);

  bsp_display_unlock();

  lv_timer_create(update_gauge, 50, NULL);
}