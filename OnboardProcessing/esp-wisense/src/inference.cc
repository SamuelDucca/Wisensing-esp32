#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_wisense.h"
#include "esp_wisense_internal.h"
#include "preprocessing.h"

namespace {
tflite::MicroInterpreter *interpreter;
TfLiteTensor *input_tensor;
TfLiteTensor *output_tensor;

constexpr int kTensorArenaSize = 20 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
} // namespace

TfLiteStatus model_setup(const uint8_t *model_ptr) {
  TfLiteStatus status;
  static const tflite::Model *model = tflite::GetModel(model_ptr);

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model provided is schema version %d not equal to supported "
                "version %d.",
                model->version(), TFLITE_SCHEMA_VERSION);
    return kTfLiteError;
  }

  static tflite::MicroMutableOpResolver<3> resolver;

  status = resolver.AddFullyConnected();
  if (status != kTfLiteOk) {
    return status;
  }
  status = resolver.AddLogistic();
  if (status != kTfLiteOk) {
    return status;
  }
  status = resolver.AddQuantize();
  if (status != kTfLiteOk) {
    return status;
  }

  static tflite::MicroInterpreter s_interpreter{model, resolver, tensor_arena,
                                                kTensorArenaSize};
  interpreter = &s_interpreter;

  status = interpreter->AllocateTensors();
  if (status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return status;
  }

  input_tensor  = interpreter->input(0);
  output_tensor = interpreter->output(0);

  return status;
}

TfLiteStatus run_inference(const float *x, float *y) {
  static float inv_scale = 1.0f / input_tensor->params.scale;
  static uint8_t *input  = tflite::GetTensorData<uint8_t>(input_tensor);
  static uint8_t *output = tflite::GetTensorData<uint8_t>(output_tensor);

  TfLiteStatus invoke_status;

  assert(x && y);

  for (int i = 0; i < PCA_COMPONENTS; i++) {
    input[i] = static_cast<uint8_t>(x[i] * inv_scale +
                                    input_tensor->params.zero_point);
  }

  invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    MicroPrintf("Invoke failed\n");
    return invoke_status;
  }

  *y = static_cast<float>(*output - output_tensor->params.zero_point) *
       output_tensor->params.scale;
  return invoke_status;
}
