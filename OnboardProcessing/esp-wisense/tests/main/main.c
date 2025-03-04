#include "esp_log.h"
#include "nvs_flash.h"

#include "../private/esp_wisense_internal.h"
#include "esp_wisense.h"
#include "esp_wisense_connection.h"
#include "fixed_csi.h"
#include "model.h"

const char *TAG = "Test App";

static void detection(float *score) {
  ESP_LOGI(TAG, "detection score = %.6f", *score);
  if (*score >= 0.5) {
    ESP_LOGI(TAG, "person detected");
  }
}

void test_task(void *pv) {
  for (size_t i = 0; i < 100; i++) {
    wifi_csi_rx_cb(pv, &csi[i]);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  vTaskSuspend(NULL);
}

void app_main() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  model_setup(g_model);
  register_callback(detection);

  esp_wisense_create_tasks_internal();
  xTaskCreate(test_task, "Detection Test", 2024, g_csi_buffer, 5, NULL);
}
