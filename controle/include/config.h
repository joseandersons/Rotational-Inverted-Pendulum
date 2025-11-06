#pragma once
#include <Arduino.h>

#define CORE_CTRL 0
#define CORE_IO   1

#define PRIO_CONTROL     (configMAX_PRIORITIES-2)
#define PRIO_ENCODER     (configMAX_PRIORITIES-3)
#define PRIO_MOTOR       (configMAX_PRIORITIES-3)

#define PERIOD_CONTROL_MS 2
#define PERIOD_ENCODER_MS 2

#define IN1 19
#define IN2 18
#define ENA 25
#define ENCA 34
#define ENCB 35

#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CH 0
#define DUTY_MAX ((1<<PWM_RES)-1)

#define CPR 2400.0f
#define GEAR 1.0f
#define TAU (2.0f*(float)PI)

#define KP_DEFAULT 40.0f
#define KI_DEFAULT 0.0f
#define KD_DEFAULT 0.2f

#define U_SAT 60.0f
#define I_BAND_DEG 8.0f
#define SETPOINT_DEG 0.0f

#define DEADBAND_DUTY 3
#define KICK_DUTY 18
#define KICK_MS 35

#define WATCHDOG_MS 60