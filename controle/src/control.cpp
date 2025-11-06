#include "control.h"
#include "config.h"
#include "encoder.h"
#include "tipos.h"
#include "filas.h"
#include "ang.h"
#include "params.h"

static TaskHandle_t th = nullptr;
static float i_term = 0.0f, prev_err = 0.0f;
static float zero_offset = 0.0f;

static float thd_filt  = 0.0f;   
static float theta_prev = 0.0f;  
static uint32_t last_tx = 0;    

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

    TickType_t last = xTaskGetTickCount();

    for(;;){
      vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_CONTROL_MS));

      Obs o{}; 
      encoder_get_obs(&o);

      float theta = wrapPi(o.theta_deg*(float)PI/180.0f - zero_offset);
      float theta_deg = wrap180(theta*180.0f/(float)PI);

      float err = SETPOINT_DEG - theta_deg;

      static uint32_t prev_ms = o.stamp_ms;
      float dt = (o.stamp_ms - prev_ms) * 1e-3f; 
      if (dt <= 0) dt = 1e-3f; 
      prev_ms = o.stamp_ms;

      // Ganhos atuais (tempo real)
      PIDGains g = params_get();
      float kp = g.kp, ki = g.ki, kd = g.kd;

      // PID
      if(fabsf(err) < I_BAND_DEG) i_term += ki*err*dt; else i_term = 0.0f;
      float d_term = kd * (err - prev_err) / dt;
      float p_term = kp * err;
      float u = p_term + i_term + d_term;

      // Saturação
      if(u >  U_SAT) u =  U_SAT;
      if(u < -U_SAT) u = -U_SAT;

      // Duty
      float mag = fabsf(u)/U_SAT; 
      if(mag < 0) mag = 0; 
      if(mag > 1) mag = 1;
      int16_t duty = (int16_t)lroundf(mag * DUTY_MAX);
      if(fabsf(err) < 0.8f && fabsf(u) < (0.10f*U_SAT)) duty = 0;
      bool kick = (duty != 0);
      int16_t duty_signed = (u >= 0.0f ? duty : -duty);

      // Envio ao motor
      MotorCmd cmd{ duty_signed, kick };
      xQueueSend(qMotor, &cmd, 0);

      // θ̇ (deg/s) – filtrado para telemetria/visualização
      float thd_inst = (theta_deg - theta_prev) / dt;
      theta_prev = theta_deg;
      const float ALPHA = 0.85f;  // mais alto = mais suave
      thd_filt = ALPHA * thd_filt + (1.0f - ALPHA) * thd_inst;

      // ---- Telemetria: empacota e envia p/ a fila (não bloqueia)
    static uint32_t last_tx = 0;
    if (millis() - last_tx >= 20) {
      last_tx = millis();

      TelePkt pkt;
      pkt.t_ms   = o.stamp_ms;
      pkt.th_c2  = (int16_t)lroundf(theta_deg * 100.0f);
      pkt.sp_c2  = (int16_t)lroundf(SETPOINT_DEG * 100.0f);
      pkt.e_c2   = (int16_t)lroundf(err * 100.0f);
      pkt.thd_c2 = (int16_t)lroundf(thd_filt * 100.0f);
      pkt.u_c2   = (int16_t)lroundf(u * 100.0f);
      pkt.p_c2   = (int16_t)lroundf(p_term * 100.0f);
      pkt.i_c2   = (int16_t)lroundf(i_term * 100.0f);
      pkt.d_c2   = (int16_t)lroundf(d_term * 100.0f);
      pkt.duty   = duty_signed;
      pkt.sat    = (uint8_t)(fabsf(u) >= U_SAT);
      pkt.kick   = (uint8_t)kick;
      pkt.dt_ms  = (uint16_t)lroundf(dt * 1000.0f);

      xQueueSend(qTel, &pkt, 0);  
    }
      prev_err = err;
    }

  }, "thCONTROL", 4096, nullptr, PRIO_CONTROL, &th, CORE_CTRL);
}
