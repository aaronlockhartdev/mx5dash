#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "canbus.h"
#include "ui.h"

#define TAG "ui"

static lv_obj_t *rpm_arc;
static lv_obj_t *stale_label;
static lv_obj_t *stale_screen;
static lv_obj_t *gauge_screen;
static lv_anim_t rpm_anim;
static lv_anim_t *rpm_anim_run;

static void rpm_deleted_cb(lv_anim_t *anim) {
  (void)anim;
  rpm_anim_run = NULL;
}

static void update_gauge(lv_timer_t *timer) {
  (void)timer;

  canbus_data_t *data = canbus_get_latest();

  bsp_display_lock(-1);

  if (canbus_is_stale()) {
    lv_obj_clear_flag(stale_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(gauge_screen, LV_OBJ_FLAG_HIDDEN);
  }
  else {
    lv_obj_add_flag(stale_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(gauge_screen, LV_OBJ_FLAG_HIDDEN);

    int32_t cur = lv_arc_get_value(rpm_arc);
    if (cur != data->rpm) {
      if (rpm_anim_run) lv_anim_delete(rpm_anim_run->var, rpm_anim_run->exec_cb);
      lv_anim_set_var(&rpm_anim, rpm_arc);
      lv_anim_set_values(&rpm_anim, cur, data->rpm);
      lv_anim_set_exec_cb(&rpm_anim, (lv_anim_exec_xcb_t)lv_arc_set_value);
      lv_anim_set_duration(&rpm_anim, 100);
      lv_anim_set_deleted_cb(&rpm_anim, rpm_deleted_cb);
      rpm_anim_run = lv_anim_start(&rpm_anim);
    }
  }

  bsp_display_unlock();
}

void ui_init(void) {
  bsp_display_lock(-1);

  gauge_screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(gauge_screen, lv_pct(100), lv_pct(100));
  lv_obj_clear_flag(gauge_screen, LV_OBJ_FLAG_SCROLLABLE);

  rpm_arc = lv_arc_create(gauge_screen);
  lv_arc_set_min_value(rpm_arc, 0);
  lv_arc_set_max_value(rpm_arc, 9000);
  lv_arc_set_bg_angles(rpm_arc, 0, 270);
  lv_arc_set_rotation(rpm_arc, 135);
  lv_arc_set_value(rpm_arc, 0);
  lv_obj_remove_style(rpm_arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(rpm_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(rpm_arc, lv_pct(85), lv_pct(85));
  lv_obj_center(rpm_arc);

  stale_screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(stale_screen, lv_pct(100), lv_pct(100));
  lv_obj_clear_flag(stale_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(stale_screen, LV_OBJ_FLAG_HIDDEN);

  stale_label = lv_label_create(stale_screen);
  lv_label_set_text(stale_label, "No CAN data");
  lv_obj_center(stale_label);

  bsp_display_unlock();

  lv_anim_init(&rpm_anim);
  lv_timer_create(update_gauge, 100, NULL);

  ESP_LOGI(TAG, "UI initialized");
}