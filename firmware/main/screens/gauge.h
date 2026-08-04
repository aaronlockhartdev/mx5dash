#pragma once

#include "canbus.h"

void gauge_screen_build(void);
void gauge_screen_update(canbus_data_t *data);
lv_obj_t *gauge_screen_get(void);