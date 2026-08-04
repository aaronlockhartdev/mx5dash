#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "canbus.h"
#include "ui.h"
#include "screens/gauge.h"
#include "sdkconfig.h"

#define TAG "ui"

typedef struct {
  lv_obj_t *screen;
  void (*update)(canbus_data_t *data);
} screen_t;

static screen_t current;
static lv_timer_t *ui_timer;

static lv_obj_t *stale_screen;

static screen_t screens[] = {
  { NULL, NULL },           // 0: stale
  { NULL, gauge_screen_update }, // 1: gauge
};

static void build_stale_screen(void) {
  stale_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(stale_screen, lv_color_black(), 0);
  lv_obj_clear_flag(stale_screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *label = lv_label_create(stale_screen);
  lv_label_set_text(label, "No CAN data");
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_center(label);

  screens[0].screen = stale_screen;
}

static void update_ui(lv_timer_t *timer) {
  (void)timer;

  bsp_display_lock(-1);

  int next_idx;
#ifdef CONFIG_DEBUG_ALWAYS_SHOW_GAUGE
  next_idx = 1;
#else
  next_idx = canbus_is_stale() ? 0 : 1;
#endif

  screen_t *next = &screens[next_idx];
  if (current.screen != next->screen) {
    lv_screen_load(next->screen);
    current = *next;
  }

  if (current.update) {
    current.update(canbus_get_latest());
  }

  bsp_display_unlock();
}

void ui_init(void) {
  bsp_display_lock(-1);

  build_stale_screen();
  gauge_screen_build();
  screens[1].screen = gauge_screen_get();

  int init_idx;
#ifdef CONFIG_DEBUG_ALWAYS_SHOW_GAUGE
  init_idx = 1;
#else
  init_idx = 0;
#endif

  lv_screen_load(screens[init_idx].screen);
  current = screens[init_idx];

  bsp_display_unlock();

  ui_timer = lv_timer_create(update_ui, 100, NULL);

  ESP_LOGI(TAG, "UI initialized");
}