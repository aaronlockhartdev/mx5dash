#include "lvgl.h"
#include "canbus.h"
#include "screens/gauge.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define SHIFT_CENTER_X      233
#define SHIFT_CENTER_Y      233
#define SHIFT_RPM           7500
#define YELLOW_START_RPM    6000
#define GREEN_START_RPM     4000

#define SHIFT_LED_COUNT     7
#define SHIFT_LED_RADIUS    210
#define SHIFT_LED_SIZE      32
#define SHIFT_LED_START_DEG 40
#define SHIFT_LED_END_DEG   140

#define NUM_GAUGE_CELLS     4

typedef struct {
  const char *label;
  size_t value_offset;
  const char *format;
  lv_obj_t *label_obj;
} gauge_cell_t;

static lv_obj_t *screen;
static lv_obj_t *shift_leds[SHIFT_LED_COUNT];
static gauge_cell_t cells[NUM_GAUGE_CELLS];

static lv_style_t style_cell;
static lv_style_t style_label;
static lv_style_t style_value_label;
static lv_style_t style_screen_bg;

static canbus_data_t last_data;
static bool has_data = false;

static const float yellow_span_inv = 1.0f / (SHIFT_RPM - YELLOW_START_RPM);
static const float green_span_inv  = 1.0f / (YELLOW_START_RPM - GREEN_START_RPM);

static void init_styles(void) {
  lv_style_init(&style_cell);
  lv_style_set_bg_color(&style_cell, lv_color_black());
  lv_style_set_bg_opa(&style_cell, LV_OPA_COVER);
  lv_style_set_border_width(&style_cell, 0);

  lv_style_init(&style_label);
  lv_style_set_text_color(&style_label, lv_color_hex(0x888888));
  lv_style_set_text_font(&style_label, &lv_font_montserrat_20);

  lv_style_init(&style_value_label);
  lv_style_set_text_color(&style_value_label, lv_color_white());
  lv_style_set_text_font(&style_value_label, &lv_font_montserrat_38);

  lv_style_init(&style_screen_bg);
  lv_style_set_bg_color(&style_screen_bg, lv_color_black());
}

static void build_shift_light(lv_obj_t *parent) {
  int32_t cx = SHIFT_CENTER_X;
  int32_t cy = SHIFT_CENTER_Y;
  double start_rad = (double)SHIFT_LED_START_DEG * M_PI / 180.0;
  double end_rad = (double)SHIFT_LED_END_DEG * M_PI / 180.0;
  double span = end_rad - start_rad;

  for (int i = 0; i < SHIFT_LED_COUNT; i++) {
    double angle = start_rad + span * (double)i / (SHIFT_LED_COUNT - 1);
    int32_t x = cx + (int32_t)(SHIFT_LED_RADIUS * cos(angle) - SHIFT_LED_SIZE / 2);
    int32_t y = cy - (int32_t)(SHIFT_LED_RADIUS * sin(angle) - SHIFT_LED_SIZE / 2);

    shift_leds[i] = lv_led_create(parent);
    lv_obj_set_size(shift_leds[i], SHIFT_LED_SIZE, SHIFT_LED_SIZE);
    lv_obj_set_pos(shift_leds[i], x, y);
    lv_led_off(shift_leds[i]);
  }
}

static void build_value_grid(lv_obj_t *parent) {
  cells[0] = (gauge_cell_t){"OIL P", offsetof(canbus_data_t, oil_pressure), "%6.1f", NULL};
  cells[1] = (gauge_cell_t){"COOL P", offsetof(canbus_data_t, coolant_pressure), "%5.1f", NULL};
  cells[2] = (gauge_cell_t){"BATT V", offsetof(canbus_data_t, battery_voltage), "%5.2f", NULL};
  cells[3] = (gauge_cell_t){"MAP", offsetof(canbus_data_t, manifold_air_pressure), "%6.1f", NULL};

  lv_obj_t *grid = lv_obj_create(parent);
  lv_obj_set_size(grid, lv_pct(80), lv_pct(40));
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -100);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  for (int i = 0; i < NUM_GAUGE_CELLS; i++) {
    lv_obj_t *cell = lv_obj_create(grid);
    lv_obj_set_size(cell, lv_pct(48), lv_pct(48));
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cell, &style_cell, 0);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl = lv_label_create(cell);
    lv_label_set_text(lbl, cells[i].label);
    lv_obj_add_style(lbl, &style_label, 0);

    lv_obj_t *val = lv_label_create(cell);
    lv_label_set_text(val, "--");
    lv_obj_add_style(val, &style_value_label, 0);
    cells[i].label_obj = val;
  }
}

static void update_shift_light(canbus_data_t *data) {
  int mid = SHIFT_LED_COUNT / 2;
  uint8_t pairs_on = 0;

  if (data->rev_limiter || data->rpm >= SHIFT_RPM) {
    pairs_on = mid + 1;
  } else if (data->rpm >= YELLOW_START_RPM) {
    float t = (float)(data->rpm - YELLOW_START_RPM) * yellow_span_inv;
    pairs_on = mid / 2 + (uint8_t)(t * (mid / 2 + 1));
    if (pairs_on > mid + 1) pairs_on = mid + 1;
  } else if (data->rpm >= GREEN_START_RPM) {
    float t = (float)(data->rpm - GREEN_START_RPM) * green_span_inv;
    pairs_on = (uint8_t)(t * (mid / 2));
    if (pairs_on < 1 && data->rpm > 0) pairs_on = 1;
  }

  for (int i = 0; i < SHIFT_LED_COUNT; i++) {
    int dist = (i <= mid) ? i : (SHIFT_LED_COUNT - 1 - i);
    bool on = (dist < pairs_on);

    if (on) {
      lv_color_t c;
      if (dist == mid) c = lv_color_hex(0xff0000);
      else if (dist >= mid / 2) c = lv_color_hex(0xdddd00);
      else c = lv_color_hex(0x00cc00);

      lv_led_on(shift_leds[i]);
      lv_obj_set_style_bg_color(shift_leds[i], c, LV_PART_MAIN);
    } else {
      lv_led_off(shift_leds[i]);
    }
  }
}

static void update_values(canbus_data_t *data) {
  for (int i = 0; i < NUM_GAUGE_CELLS; i++) {
    float *v = (float *)((char *)data + cells[i].value_offset);
    lv_label_set_text_fmt(cells[i].label_obj, cells[i].format, *v);
  }
}

void gauge_screen_build(void) {
  init_styles();

  screen = lv_obj_create(NULL);
  lv_obj_add_style(screen, &style_screen_bg, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  build_shift_light(screen);
  build_value_grid(screen);
}

void gauge_screen_update(canbus_data_t *data) {
  bool changed = !has_data || memcmp(data, &last_data, sizeof(*data)) != 0;
  if (!changed) return;

  last_data = *data;
  has_data = true;

  update_shift_light(data);
  update_values(data);
}

lv_obj_t *gauge_screen_get(void) {
  return screen;
}