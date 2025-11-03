#include "filas.h"

QueueHandle_t qMotor=nullptr;

void filas_init(){
  qMotor = xQueueCreate(8, sizeof(MotorCmd));
}
