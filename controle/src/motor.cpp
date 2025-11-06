#include "motor.h"
#include "config.h"
#include "filas.h"
#include "tipos.h"

static TaskHandle_t th = nullptr;

static void motor_write_signed(int16_t dutySigned, bool kick) {
  if (dutySigned == 0) { ledcWrite(PWM_CH, 0); return; }
  if (dutySigned > 0) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
  else { digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); }
  uint16_t mag = abs(dutySigned);
  if (mag > 0 && mag < DEADBAND_DUTY) mag = DEADBAND_DUTY;
  if (mag > DUTY_MAX) mag = DUTY_MAX;
  if (kick && mag == DEADBAND_DUTY) { ledcWrite(PWM_CH, KICK_DUTY); vTaskDelay(pdMS_TO_TICKS(KICK_MS)); }
  ledcWrite(PWM_CH, mag);
}

static void motor_task(void*) {180
  MotorCmd cmd{};
  for (;;) {
    if (xQueueReceive(qMotor, &cmd, pdMS_TO_TICKS(WATCHDOG_MS)) == pdTRUE) {
      motor_write_signed(cmd.duty_signed, cmd.kick);
    } else {
      ledcWrite(PWM_CH, 0);
    }
  }
}

void motor_init() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, PWM_CH);
  ledcWrite(PWM_CH, 0);
}

void motor_task_start() {
  xTaskCreatePinnedToCore(motor_task, "thMOTOR", 3072, nullptr, PRIO_MOTOR, &th, CORE_IO);
}
