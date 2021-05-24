/*
  Referenced:
   https://github.com/arduino/ArduinoTensorFlowLiteTutorials/blob/master/GestureToEmoji/ArduinoSketches/IMU_Classifier/IMU_Classifier.ino
   https://www.digikey.com/en/maker/projects/intro-to-tinyml-part-2-deploying-a-tensorflow-lite-model-to-arduino/59bf2d67256f4b40900a3fa670c14330  
 */
 
#include <Arduino_OV767X.h>
#include <TTS.h>

#include "TensorFlowLite.h"
#include "tensorflow/lite/experimental/micro/kernels/micro_ops.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/experimental/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/version.h"

/* Include the model here */
#include "OseeR_model.h"

byte imgdata[176 * 144 * 2]; // using QCIF
int bytesperframe;
int outputnums[10];
byte maxnum;

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* model_input = nullptr;
  TfLiteTensor* model_output = nullptr;

  constexpr int kTensorArenaSize = 8 * 1024;      //can probably make this smaller once it works
  uint8_t tensor_arena[kTensorArenaSize];
} // TFL Namespace 

void setup() {
  maxnum = 0;
  char text[50];

  text2speech.setPitch(4);
  
  Camera.begin(QCIF, RGB565, 1);
  bytesperframe = Camera.width() * Camera.height() * Camera.bytesPerPixel();
  
    // Set up logging (will report to Serial, even within TFLite functions)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure
  // read in the model from thermo_model.h
  model = tflite::GetModel(OseeR_model);
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    error_reporter->Report("Model version does not match schema");
    while(1);
  }

  static tflite::MicroMutableOpResolver micro_mutable_op_resolver;
  micro_mutable_op_resolver.AddBuiltin(
    tflite::BuiltinOperator_FULLY_CONNECTED,
    tflite::ops::micro::Register_FULLY_CONNECTED(),
    1, 9);
  
  // Build an interpreter to run the model
  static tflite::MicroInterpreter static_interpreter(
    model, micro_mutable_op_resolver, tensor_arena, 
    kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate model input and output buffers (tensors) to pointers
  model_input = interpreter->input(0);
  model_output = interpreter->output(0);
}

void loop() {
  /* Running at 176 x 144, rgb565 */
  Camera.readFrame(imgdata);
  
  /* Function to downsample image */
  // We need to add a function here to downsample the image

  /* Function to convert image to greyscale */
  // We need to add a function here to convert to greyscale, easier to process


    for(int i = 0; i < bytesperframe; i++)
    {
      model_input->data.f[i] = imgdata[i];
    }
  
    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on input\n");
    }

    /* Grab output values */
    for(int i = 0; i < 10; i++)
    {
      outputnums[i] = model_output->data.f[i];
      if(outputnums[i] > outputnums[maxnum])
      {
        maxnum = i;
      }
    }
  /* Print out the character it probably is, and the percentage certainty */
  Serial.println("Character: " + maxnum + " percentage: " + outputnums[maxnum]);

  strcpy(text, "%d", maxnum);
  text2speech.say(text);
  delay(1000);
}
