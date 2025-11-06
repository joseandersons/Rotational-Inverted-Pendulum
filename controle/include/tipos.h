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

typedef struct {
  uint32_t t_ms;   // timestamp (ms)
  int16_t th_c2;   // θ *100 (centideg)
  int16_t sp_c2;   // sp *100
  int16_t e_c2;    // erro *100
  int16_t thd_c2;  // θ̇ *100 (deg/s)
  int16_t u_c2;    // u *100
  int16_t p_c2;    // P *100
  int16_t i_c2;    // I *100
  int16_t d_c2;    // D *100
  int16_t duty;    // duty assinado
  uint8_t sat;     // 0/1
  uint8_t kick;    // 0/1
  uint16_t dt_ms;  // dt em ms
} TelePkt;
