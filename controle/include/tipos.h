#pragma once
#include <Arduino.h>

struct Obs {
  float theta_deg;
  float omega_dps;
  uint32_t stamp_ms;
};

struct MotorCmd {
  int16_t duty_signed;
  bool kick;
};
