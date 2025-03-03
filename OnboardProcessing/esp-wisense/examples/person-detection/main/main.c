#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_wisense.h"
#include "esp_wisense_connection.h"
#include "model.h"

const char *TAG = "Person Detection";

#include "../../../private/esp_wisense_internal.h"
#include "test.h"
void vTestTask(void *pv) {
  for (size_t i = 0; i < 100; i++) {
    wifi_csi_rx_cb(pv, &csi[i]);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskSuspend(NULL);
}

static void detection(float *score) {
  ESP_LOGI(TAG, "detection score = %.6f", *score);
  if (*score >= 0.5) {
    ESP_LOGI(TAG, "person detected");
  }
}

void app_main() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /*wifi_init();
  EventBits_t bits = xEventGroupWaitBits(g_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(
        TAG,
        "Failed to connect to Wi-Fi. Verify configuration and network access.");
    esp_restart();
  }
  wifi_csi_init(false);*/

  model_setup(g_model);
  register_callback(detection);

  esp_wisense_create_tasks();
  // esp_wisense_enable(true);

  xTaskCreate(vTestTask, "TESTING", 2024, g_csi_buffer, 5, NULL);
}
