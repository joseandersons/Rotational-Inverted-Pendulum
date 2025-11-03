#pragma once
#include <Arduino.h>
#include "tipos.h"

void encoder_isr_init();
int32_t encoder_get_count_isr_safe();

void encoder_task_start();
bool encoder_get_obs(Obs* out);
float encoder_get_angle_rad_now();
