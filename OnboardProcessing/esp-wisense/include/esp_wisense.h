#pragma once

#include <stdbool.h>

#include "tensorflow/lite/core/c/c_api_types.h"

#if CONFIG_DEBUG_PRINT
#include "rom/ets_sys.h"
#include "xtensa/hal.h"
#define PROFILE_START(id) uint32_t __profile_start_##id = xthal_get_ccount()
#define PROFILE_END(id)                                                        \
  do {                                                                         \
    uint32_t __profile_end_##id = xthal_get_ccount();                          \
    uint32_t __elapsed_cycles   = __profile_end_##id - __profile_start_##id;   \
                                                                               \
    ets_printf(#id ",%" PRIu32 ",%" PRIu32 "\n", __elapsed_cycles,             \
               (__elapsed_cycles / CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ));          \
  } while (0)
#else
// clang-format off
#define PROFILE_START(id) do {} while(0)
#define PROFILE_END(id)   do {} while(0)
// clang-format on
#endif

typedef void (*inference_cb)(float *);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Initialize the TFLite model.
 *
 * @param[in] modrl_ptr  Pointer to model array in flash.
 *
 * @return
 *     - kTfLiteOk
 *     - kTfLiteError
 */
TfLiteStatus model_setup(const uint8_t *model_ptr);

/**
 * @brief Create necessary tasks for CSI processing and inference.
 */
void esp_wisense_create_tasks(void);

/**
 * @brief     Enable/Disable AP ping and CSi extraction.
 *
 * @param[in] en  true - enable, false - disable.
 */
void esp_wisense_enable(bool en);

/**
 * @brief     Register inference callback.
 *
 * @param[in] cb  true - enable, false - disable.
 */
void register_callback(inference_cb cb);

#ifdef __cplusplus
}
#endif
