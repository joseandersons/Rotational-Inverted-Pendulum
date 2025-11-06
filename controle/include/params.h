#pragma once
#include <Arduino.h>

struct PIDGains { float kp, ki, kd; };

void params_begin();                
PIDGains params_get();              
void params_set(float kp, float ki, float kd);
void params_save();                  

inline float pid_kp(){ return params_get().kp; }
inline float pid_ki(){ return params_get().ki; }
inline float pid_kd(){ return params_get().kd; }
