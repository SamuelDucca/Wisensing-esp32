#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "freertos/ringbuf.h"

#include "esp_wisense_connection.h"
#include "esp_wisense_internal.h"

static const char *TAG = "WISENSE-RX";

#ifdef CONFIG_WIFI_STA
static int8_t s_retry_count;
EventGroupHandle_t g_wifi_event_group;

static void event_handler(void *, esp_event_base_t, int32_t, void *);
#endif

void wifi_init(void) {
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(esp_netif_init());
#ifdef CONFIG_WIFI_STA
  esp_netif_create_default_wifi_sta();
#else
  esp_netif_create_default_wifi_ap();
#endif

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#ifdef CONFIG_WIFI_STA
  g_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

  wifi_config_t wifi_config =
  {.sta = {
#else
  wifi_config_t wifi_config = {.ap = {
                                   .channel = CONFIG_WIFI_CHANNEL,
#endif
       .ssid     = CONFIG_WIFI_SSID,
       .password = CONFIG_WIFI_PASSWORD,
   } };

#ifdef CONFIG_WIFI_STA
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
#else
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
#endif

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_csi_init(bool is_promiscuous) {
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(is_promiscuous));

  wifi_csi_config_t csi_config = {
      .lltf_en           = true,
      .htltf_en          = false,
      .stbc_htltf2_en    = false,
      .ltf_merge_en      = false,
      .channel_filter_en = false,
      .manu_scale        = false,
      .shift             = false,
  };

  ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
  ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_rx_cb, NULL));
}

void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info) {
  csi_frame *frame;

  if (!info || !info->buf) {
    ESP_LOGW(TAG, "<%s> wifi_csi_cb", esp_err_to_name(ESP_ERR_INVALID_ARG));
    return;
  }
  const wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;

  UBaseType_t res = xRingbufferSendAcquire(g_csi_buffer, (void **)&frame,
                                           sizeof(csi_frame), 0);
  if (res != pdTRUE) {
    ESP_LOGW(TAG,
             "Failed to acquire memory for CSI frame > slow data consumption");
    return;
  }

  /*
   * According to the ESP-IDF Wi-Fi driver information, CSI data is as follows:
   *
   * |-----------------------|------------------|-------------------------|
   * |   Secondary Channel   |       None       |       Below/Above       |
   * |-----------------------|------------------|-------------------------|
   * |      L-LTF order      |   0~31, -32~-1   |       0~63/-64~-1       |
   * |   Valid Subcarriers   |    [-26, 26]*    |   [6, 58]/[-58, -6]**   |
   * |-----------------------|------------------|-------------------------|
   *
   * * Except for subcarrier 0
   * ** Except for indices 32/-32 (subcarrier 0)
   */
  if (rx_ctrl->secondary_channel == WIFI_SECOND_CHAN_NONE) {
    const int8_t csi_offset = info->first_word_invalid ? 4 : 2;

    // Write 0+i0 in subcarrier 1 (index 26) and overwrite it if
    // info->first_word_invalid is not set
    ((int16_t *)frame->csi)[26] = 0;
    memcpy(frame->csi, info->buf + 76, 52);
    memcpy(frame->csi + 50 + csi_offset, info->buf + csi_offset,
           54 - csi_offset);
  } else {
    memcpy(frame->csi, info->buf + 12, 52);
    memcpy(frame->csi + 52, info->buf + 66, 52);
  }

  frame->rssi = rx_ctrl->rssi;

  res = xRingbufferSendComplete(g_csi_buffer, frame);
  if (res != pdTRUE) {
    ESP_LOGE(TAG, "Failed to send CSI frame");
  }
}

#ifdef CONFIG_WIFI_STA
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_count < CONFIG_WIFI_MAX_RETRY) {
      esp_wifi_connect();
      s_retry_count++;

      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(g_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    s_retry_count = 0;
    xEventGroupSetBits(g_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}
#endif
