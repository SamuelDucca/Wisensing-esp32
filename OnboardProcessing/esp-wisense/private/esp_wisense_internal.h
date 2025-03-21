#pragma once

#include <stdint.h>

#include "esp_wifi_types_generic.h"
#include "freertos/ringbuf.h"
#include "tensorflow/lite/core/c/c_api_types.h"

/**
 * @brief filtered CSI and RSSI of frame
 */
typedef struct {
  int8_t csi[104];
  int8_t rssi;
} __attribute__((packed)) csi_frame;

extern QueueHandle_t g_feature_queue;
extern RingbufHandle_t g_csi_buffer;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Callback with Wi-Fi PHY data
 *
 * @attention  1. This API serves only to facilitate testing.
 *
 * @param[in] ctx   Pointer to context variable.
 * @param[in] info  Pointer to PHY data information, including CSI.
 */
void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info);

/**
 * @brief  Create the processing and inference task
 */
void esp_wisense_create_tasks_internal(void);

/**
 * @brief      Runs inference on the provided input features.
 *
 * @attention  1. Global model must be set with `model_setup` before this API is
 *                called.
 *
 * @param[in]  x  Pointer to feature array.
 * @param[out] y  Pointer to inference score.
 */
TfLiteStatus run_inference(const float *x, float *y);

#ifdef __cplusplus
}
#endif
