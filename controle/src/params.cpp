                                                                        #include <Preferences.h>
#include "params.h"
#include "config.h"

static Preferences prefs;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile float g_kp = KP_DEFAULT;
static volatile float g_ki = KI_DEFAULT;
static volatile float g_kd = KD_DEFAULT;

void params_begin() {
  prefs.begin("pid", true);
  float kp = prefs.getFloat("kp", KP_DEFAULT);
  float ki = prefs.getFloat("ki", KI_DEFAULT);
  float kd = prefs.getFloat("kd", KD_DEFAULT);
  prefs.end();
  portENTER_CRITICAL(&mux);
  g_kp = kp; g_ki = ki; g_kd = kd;
  portEXIT_CRITICAL(&mux);
}

PIDGains params_get() {
  PIDGains r;
  portENTER_CRITICAL(&mux);
  r.kp = g_kp; r.ki = g_ki; r.kd = g_kd;
  portEXIT_CRITICAL(&mux);
  return r;
}

void params_set(float kp, float ki, float kd) {
  portENTER_CRITICAL(&mux);
  g_kp = kp; g_ki = ki; g_kd = kd;
  portEXIT_CRITICAL(&mux);
  params_save();
}

void params_save() {
  PIDGains g = params_get();
  prefs.begin("pid", false);
  prefs.putFloat("kp", g.kp);
  prefs.putFloat("ki", g.ki);
  prefs.putFloat("kd", g.kd);
  prefs.end();
}
