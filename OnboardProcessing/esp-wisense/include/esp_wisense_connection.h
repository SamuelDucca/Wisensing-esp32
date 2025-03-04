#pragma once

#include <stdbool.h>

#ifdef CONFIG_WIFI_STA
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern EventGroupHandle_t g_wifi_event_group;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Wi-Fi module.
 *        Select the operation mode based on CONFIG_WIFI_MODE. Available
 *        configurations are CONFIG_WIFI_STA and CONFIG_WIFI_AP.
 */
void wifi_init(void);

/**
 * @brief     Initialize Wi-Fi Channel State Information (CSI).
 *            Enable L-LTF extraction for the connected AP and register a buffer
 *            for outgoing CSI.
 *
 * @attention 1. This API should only be called after `wifi_init`.
 *
 * @param[in] is_promiscuous  Enable promiscuous traffic monitoring to collect
 *                            CSI.
 */
void wifi_csi_init(bool is_promiscuous);

#ifdef __cplusplus
}
#endif
