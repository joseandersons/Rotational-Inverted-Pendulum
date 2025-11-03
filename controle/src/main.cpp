#include <Arduino.h>
#include "config.h"
#include "filas.h"
#include "encoder.h"
#include "motor.h"
#include "control.h"

void setup(){
  Serial.begin(115200);
  filas_init();
  encoder_isr_init();
  motor_init();
  vTaskDelay(pdMS_TO_TICKS(300));
  encoder_task_start();
  motor_task_start();
  control_task_start();
}

void loop(){
  vTaskDelay(pdMS_TO_TICKS(1000));
}
