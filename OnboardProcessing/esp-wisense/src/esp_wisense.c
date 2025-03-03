#include <math.h>

#include "dsps_addc.h"
#include "dsps_dotprod.h"
#include "dsps_sub.h"
#include "esp_wifi.h"
#include "ping/ping_sock.h"
#include "rom/ets_sys.h"

#include "esp_wisense.h"
#include "esp_wisense_internal.h"
#include "freertos/ringbuf.h"
#include "preprocessing.h"

#define QUEUE_SIZE 5

static const char *TAG = "WISENSE-PING";

static inference_cb s_task_cb;
static esp_ping_handle_t s_ping_handle;

QueueHandle_t g_feature_queue;
RingbufHandle_t g_csi_buffer;

static void processing_task(void *);
static void inference_task(void *);

inline void register_callback(inference_cb cb) { s_task_cb = cb; }

void esp_wisense_create_tasks() {
  g_csi_buffer    = xRingbufferCreateNoSplit(sizeof(csi_frame), 10);
  g_feature_queue = xQueueCreate(QUEUE_SIZE, PCA_COMPONENTS * sizeof(float));

  /*  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.count             = 0;
    ping_config.interval_ms       = 1000 / CONFIG_SAMPLE_RATE;
    ping_config.task_stack_size   = 3072;
    ping_config.data_size         = 1;

    esp_netif_ip_info_t local_ip;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"),
                          &local_ip);
    ESP_LOGI(TAG, "got ip:" IPSTR ", gw: " IPSTR, IP2STR(&local_ip.ip),
             IP2STR(&local_ip.gw));
    ping_config.target_addr.u_addr.ip4.addr = ip4_addr_get_u32(&local_ip.gw);
    ping_config.target_addr.type            = ESP_IPADDR_TYPE_V4;

    esp_ping_callbacks_t cbs = {0};
    esp_ping_new_session(&ping_config, &cbs, &s_ping_handle);*/

  xTaskCreatePinnedToCore(processing_task, "CSI Processing", 4096, NULL, 10,
                          NULL, PRO_CPU_NUM);
  xTaskCreatePinnedToCore(inference_task, "Model Inference", 4096, s_task_cb,
                          10, NULL, APP_CPU_NUM);
}

void esp_wisense_enable(bool en) {
  if (!s_ping_handle) {
    ESP_LOGW(TAG, "Attempt of enabling/disabling WiSense without "
                  "properly initializing corresponding tasks.\n"
                  "Call `esp_wisense_create_tasks` first");
    return;
  }

  if (en) {
    esp_ping_start(s_ping_handle);
  } else {
    esp_ping_stop(s_ping_handle);
  }

  ESP_ERROR_CHECK(esp_wifi_set_csi(en));
}

static void processing_task(void *params) {
  float csi_db[SUBCARRIER_COUNT];

  ets_printf("step,cycles,time");
  while (true) {
    float pca_components[PCA_COMPONENTS] = {0};

    for (size_t frame = 0; frame < FRAME_WINDOW_SIZE; frame++) {
      size_t received_size;
      float csi_power_sum = 0;

      csi_frame *csi = (csi_frame *)xRingbufferReceive(
          g_csi_buffer, &received_size, portMAX_DELAY);

      PROFILE_START(amplitude);
      for (size_t i = 0; i < SUBCARRIER_COUNT; i++) {
        const size_t step = 2 * i;
        const float imag  = (float)csi->csi[step];
        const float real  = (float)csi->csi[step + 1];

        float subcarrier_power = real * real + imag * imag;

        csi_db[i]      = 10.0f * log10f(subcarrier_power);
        csi_power_sum += subcarrier_power;
      }

      /*
       * ESP32 CSI is estimated after AGC, thus a scaling factor is needed to
       * retrieve the undistorted channel state.
       *
       * The implementation shown in https://doi.org/10.1109/JIOT.2020.3022573
       * defines scaling_factor = sqrt(10^(RSS/10) / sum(CSIi^2)).
       *
       * For the current application, CSI magnitude is taken in dB, thus the
       * scaling can be performed as db(CSI_fix) = db(CSI) + db(scaling_factor),
       * where db(scaling_factor) = RSS - db(sum(CSIi^2)) / 2.
       */
      float scaling_factor = csi->rssi - 10.0f * log10f(csi_power_sum);
      dsps_addc_f32(csi_db, csi_db, SUBCARRIER_COUNT, scaling_factor, 1, 1);
      PROFILE_END(amplitude);

      vRingbufferReturnItem(g_csi_buffer, csi);

#if (CONFIG_ROLLING_MEAN_WINDOW > 1)
      PROFILE_START(rolling_mean);
      {
        float prefix_sum[SUBCARRIER_COUNT];
        int8_t prefix_count[SUBCARRIER_COUNT];

        float current_sum    = 0.0f;
        int8_t current_count = 0;
        for (size_t i = 0; i < SUBCARRIER_COUNT; i++) {
          const float val  = csi_db[i];
          const bool valid = isfinite(val);

          if (valid) {
            current_sum   += val;
            current_count += 1;
          }
          prefix_sum[i]   = current_sum;
          prefix_count[i] = current_count;
        }

        for (size_t i = 0; i < SUBCARRIER_COUNT; i++) {
          const int8_t start = (int)(i - CONFIG_ROLLING_MEAN_WINDOW / 2) < 0
                                   ? 0
                                   : (i - CONFIG_ROLLING_MEAN_WINDOW / 2);
          const int8_t end =
              (i + (CONFIG_ROLLING_MEAN_WINDOW - 1) / 2) >= SUBCARRIER_COUNT
                  ? SUBCARRIER_COUNT - 1
                  : (i + (CONFIG_ROLLING_MEAN_WINDOW - 1) / 2);

          float sum;
          int8_t count;
          if (start == 0) {
            sum   = prefix_sum[end];
            count = prefix_count[end];
          } else {
            sum   = prefix_sum[end] - prefix_sum[start - 1];
            count = prefix_count[end] - prefix_count[start - 1];
          }
          csi_db[i] = (count > 0) ? (sum / (float)count) : NAN;
        }
      }
      PROFILE_END(rolling_mean);
#endif

      PROFILE_START(scaling);
      {
        const standard_scaler *scaler_ptr = scaler[frame];
        for (size_t i = 0; i < SUBCARRIER_COUNT; i++) {
          csi_db[i] = (csi_db[i] - scaler_ptr[i].mean) / scaler_ptr[i].std;
        }
      }
      PROFILE_END(scaling);

      PROFILE_START(pca);
      {
        const size_t frame_offset = SUBCARRIER_COUNT * frame;
        dsps_sub_f32(csi_db, pca_means[frame], csi_db, SUBCARRIER_COUNT, 1, 1,
                     1);
        for (size_t component = 0; component < PCA_COMPONENTS; component++) {
          float pca_partial_component;

          dsps_dotprod_f32(csi_db, pca_matrix[component] + frame_offset,
                           &pca_partial_component, SUBCARRIER_COUNT);
          pca_components[component] += pca_partial_component;
        }
      }
      PROFILE_END(pca);
    }
    xQueueSend(g_feature_queue, pca_components, portMAX_DELAY);
  }
}

static void inference_task(void *cb) {
  float features[PCA_COMPONENTS];

  float score;
  while (true) {
    if (xQueueReceive(g_feature_queue, features, portMAX_DELAY) == pdFALSE) {
      continue;
    }

    PROFILE_START(inference);
    if (cb && run_inference(features, &score) == kTfLiteOk) {
      ((inference_cb)cb)(&score);
    }
    PROFILE_END(inference);
  }
}
