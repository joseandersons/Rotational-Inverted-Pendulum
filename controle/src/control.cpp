#include "control.h"
#include "config.h"
#include "encoder.h"
#include "tipos.h"
#include "filas.h"
#include "ang.h"

static TaskHandle_t th=nullptr;
static float kp=KP,ki=KI,kd=KD;
static float i_term=0.0f, prev_err=0.0f;
static float zero_offset=0.0f;

static void calibrar_zero_por_fundo(){
  for(int i=10;i>0;i--){
    Serial.printf("[control] calibre no FUNDO (180°)... %ds\n", i);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  float acc=0.0f;
  const int N=200;
  for(int i=0;i<N;i++){
    acc += wrapPi(encoder_get_angle_rad_now());
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  float fundo = wrapPi(acc / (float)N);
  zero_offset = wrapPi(fundo + (float)PI);
  Serial.printf("[control] offset topo definido. fundo=%.2f°, topo=0°\n",
                wrap180(fundo * 180.0f / (float)PI));
}

void control_task_start(){
  xTaskCreatePinnedToCore([](void*){
    Serial.println("[control] start");
    calibrar_zero_por_fundo();
    Serial.println("[control] zero ok");

    TickType_t last=xTaskGetTickCount();
    uint32_t t_dbg=millis();

    for(;;){
      vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_CONTROL_MS));

      Obs o{}; encoder_get_obs(&o);

      float theta = wrapPi(o.theta_deg*(float)PI/180.0f - zero_offset);
      float theta_deg = wrap180(theta*180.0f/(float)PI);

      float err = SETPOINT_DEG - theta_deg;

      static uint32_t prev_ms = o.stamp_ms;
      float dt = (o.stamp_ms - prev_ms)*1e-3f; if(dt<=0) dt=1e-3f; prev_ms=o.stamp_ms;

      if(fabsf(err)<I_BAND_DEG) i_term += ki*err*dt; else i_term=0.0f;
      float d_term = kd*(err - prev_err)/dt;
      float p_term = kp*err;
      float u = p_term + i_term + d_term;
      if(u> U_SAT) u= U_SAT;
      if(u<-U_SAT) u=-U_SAT;

      float mag=fabsf(u)/U_SAT; if(mag<0)mag=0; if(mag>1)mag=1;
      int16_t duty=(int16_t)lroundf(mag*DUTY_MAX);
      if(fabsf(err)<0.8f && fabsf(u)<(0.10f*U_SAT)) duty=0;
      bool kick=(duty!=0);

      MotorCmd cmd{ static_cast<int16_t>(u>=0.0f? duty : -duty), kick };
      xQueueSend(qMotor,&cmd,0);

      if (millis() - t_dbg >= 200) {
        t_dbg = millis();
        Serial.printf("[control] th=%.2f e=%.2f P=%.1f I=%.1f D=%.1f u=%.1f duty=%d\n",
                      theta_deg, (SETPOINT_DEG-theta_deg), p_term, i_term, d_term, u, (int)duty);
      }

      prev_err=err;
    }
  }, "thCONTROL", 4096, nullptr, PRIO_CONTROL, &th, CORE_CTRL);
}
