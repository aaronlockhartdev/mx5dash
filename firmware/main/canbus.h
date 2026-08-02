#pragma once

#include <stdint.h>
#include <stdatomic.h>
#include "esp_err.h"
#include "esp_twai_types.h"

#define TX_PIN 4
#define RX_PIN 5
#define BITRATE 200000

typedef struct {
  bool rev_limiter;
  bool cel;
  uint16_t rpm;
  float ignition_timing;
  float injector_duty;
  float ignition_duty;
  float vehicle_speed;
  float flex;
  float throttle_position;
  float manifold_air_pressure;
  float coolant_temperature;
  float intake_temperature;
  float oil_pressure;
  float oil_temperature;
  float battery_voltage;
  float coolant_pressure;
  float lambda;
} canbus_data_t;

typedef struct {
  twai_frame_header_t header;
  uint8_t buffer[8];
} canbus_rx_t;

esp_err_t canbus_start();
canbus_data_t *canbus_get_latest(void);