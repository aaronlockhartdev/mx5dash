#include "esp_timer.h"
#include "esp_log.h"

#include "canbus.h"
#include "ecu_update.h"

#include <string.h>
#include "esp_twai_types.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TAG "canbus"
#define STALE_TIMEOUT_US 1000000

static canbus_data_t canbus_buf[2];
static atomic_uint_fast8_t canbus_buf_idx = 0;
static _Atomic int_least64_t canbus_last_update = 0;

static bool twai_rx_cb(twai_node_handle_t handle,
                       const twai_rx_done_event_data_t *edata, void *user_ctx) {
  QueueHandle_t rx_queue = (QueueHandle_t)user_ctx;
  canbus_rx_t canbus_rx;
  twai_frame_t rx_frame = {.buffer = canbus_rx.buffer, .buffer_len = 8};

  if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    canbus_rx.header = rx_frame.header;
    if (xQueueSendToFrontFromISR(rx_queue, &canbus_rx, &higher_priority_task_woken) == pdFALSE) {
      ESP_DRAM_LOGE(TAG, "queue full, dropping frame 0x%03x", rx_frame.header.id);
    }
    return higher_priority_task_woken == pdTRUE;
  }
  return false;
}

void twai_rx_task(void *pvParameters) {
  QueueHandle_t rx_queue = (QueueHandle_t)pvParameters;
  canbus_rx_t frame;
  for (;;) {
    if (xQueueReceive(rx_queue, &frame, portMAX_DELAY) == pdTRUE) {
      uint8_t idx = atomic_load(&canbus_buf_idx);
      memcpy(&canbus_buf[1 - idx], &canbus_buf[idx], sizeof(canbus_data_t));
      ecu_update(frame, &canbus_buf[1 - idx]);
      atomic_store(&canbus_buf_idx, 1 - idx);
      atomic_store(&canbus_last_update, esp_timer_get_time());
    }
  }
}

canbus_data_t *canbus_get_latest(void) {
  return &canbus_buf[atomic_load(&canbus_buf_idx)];
}

bool canbus_is_stale(void) {
  return (esp_timer_get_time() - atomic_load(&canbus_last_update)) > STALE_TIMEOUT_US;
}

esp_err_t canbus_start() {
  twai_node_handle_t node_hdl = NULL;
  twai_onchip_node_config_t node_config = {.io_cfg.rx = RX_PIN,
                                           .bit_timing.bitrate = BITRATE,
                                           .flags.enable_listen_only = true};

  twai_event_callbacks_t user_cbs = {
      .on_rx_done = twai_rx_cb,
  };

  QueueHandle_t rx_queue = xQueueCreate(16, sizeof(canbus_rx_t));

  ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
  ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs,
                                                      (void *)rx_queue));
  ESP_ERROR_CHECK(twai_node_enable(node_hdl));

  atomic_store(&canbus_last_update, esp_timer_get_time());

  xTaskCreate(twai_rx_task, "can_rx", 2048, rx_queue, 5, NULL);

  return ESP_OK;
}