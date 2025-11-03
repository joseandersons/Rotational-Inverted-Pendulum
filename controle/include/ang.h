#pragma once
#include <Arduino.h>

#ifndef TAU_ANG
#define TAU_ANG (2.0f * (float)PI)
#endif

static inline float wrapPi(float x) {
  while (x >  (float)PI) x -= TAU_ANG;
  while (x < -(float)PI) x += TAU_ANG;
  return x;
}

static inline float wrap180(float deg) {
  while (deg >= 180.0f) deg -= 360.0f;
  while (deg <  -180.0f) deg += 360.0f;
  return deg;
}
