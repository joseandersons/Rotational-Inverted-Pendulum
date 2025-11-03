#include "encoder.h"
#include "config.h"

static TaskHandle_t th=nullptr;
static SemaphoreHandle_t mtx=nullptr;
static Obs g_obs{0,0,0};

static inline float wrapPi(float x){ while(x>PI)x-=TAU; while(x<-PI)x+=TAU; return x; }

bool encoder_get_obs(Obs* out){
  if(xSemaphoreTake(mtx, pdMS_TO_TICKS(5))==pdTRUE){
    *out = g_obs;
    xSemaphoreGive(mtx);
    return true;
  }
  return false;
}

static void encoder_task(void*){
  TickType_t last=xTaskGetTickCount();
  float th_prev=0.0f;
  uint32_t t_prev=millis();
  for(;;){
    vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_ENCODER_MS));
    uint32_t t_now=millis();
    float dt=(t_now - t_prev)*1e-3f; if(dt<=0) dt=1e-3f; t_prev=t_now;
    float th = wrapPi(((float)encoder_get_count_isr_safe()/(CPR*GEAR))*TAU);
    float w  = (th - th_prev)/dt; th_prev=th;
    Obs o;
    o.theta_deg = th*180.0f/(float)PI;
    o.omega_dps = w*180.0f/(float)PI;
    o.stamp_ms  = t_now;
    if(xSemaphoreTake(mtx, 0)==pdTRUE){
      g_obs=o;
      xSemaphoreGive(mtx);
    }
  }
}

void encoder_task_start(){
  mtx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(encoder_task,"thENCODER",3072,nullptr,PRIO_ENCODER,&th,CORE_CTRL);
}
