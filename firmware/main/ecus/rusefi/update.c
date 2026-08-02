#include "../../canbus.h"
#include "database.h"
#include "esp_twai_types.h"

void update(canbus_rx_t frame, canbus_data_t *canbus_data) {
  switch (frame.header.id) {
    case DATABASE_BASE0_FRAME_ID: {
      struct database_base0_t data;
      database_base0_init(&data);
      database_base0_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->rev_limiter = data.rev_lim_act != 0;
      canbus_data->cel = data.cel_act != 0;

      break;
    }
    case DATABASE_BASE1_FRAME_ID: {
      struct database_base1_t data;
      database_base1_init(&data);
      database_base1_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->rpm = data.rpm;
      canbus_data->ignition_timing = database_base1_ignition_timing_decode(data.ignition_timing);
      canbus_data->injector_duty = database_base1_inj_duty_decode(data.inj_duty);
      canbus_data->ignition_duty = database_base1_ign_duty_decode(data.ign_duty);
      canbus_data->vehicle_speed = database_base1_vehicle_speed_decode(data.vehicle_speed);
      canbus_data->flex = database_base1_flex_pct_decode(data.flex_pct);
      break;
    }
    case DATABASE_BASE2_FRAME_ID: {
      struct database_base2_t data;
      database_base2_init(&data);
      database_base2_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->throttle_position = database_base2_tps1_decode(data.tps1);

      break;
    }
    case DATABASE_BASE3_FRAME_ID: {
      struct database_base3_t data;
      database_base3_init(&data);
      database_base3_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->manifold_air_pressure = database_base3_map_decode(data.map);
      canbus_data->coolant_temperature = database_base3_coolant_temp_decode(data.coolant_temp);
      canbus_data->intake_temperature = database_base3_intake_temp_decode(data.intake_temp);

      break;
    }

    case DATABASE_BASE4_FRAME_ID: {
      struct database_base4_t data;
      database_base4_init(&data);
      database_base4_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->oil_pressure = database_base4_oil_press_decode(data.oil_press);
      canbus_data->oil_temperature = database_base4_oil_temperature_decode(data.oil_temperature);
      canbus_data->battery_voltage = database_base4_batt_volt_decode(data.batt_volt);

      break;
    }

    case DATABASE_BASE7_FRAME_ID: {
      struct database_base7_t data;
      database_base7_init(&data);
      database_base7_unpack(&data, frame.buffer, frame.header.dlc);

      canbus_data->lambda = database_base7_lam1_decode(data.lam1);

      break;
    }
  }
}
