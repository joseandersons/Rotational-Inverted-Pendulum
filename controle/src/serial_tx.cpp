#include <Arduino.h>
#include "filas.h"
#include "tipos.h"
#include "config.h"

#ifdef TEL_UART2
  HardwareSerial Tel(2);                
  #define TEL        Tel
  #define TEL_BEGIN() Tel.begin(115200, SERIAL_8N1, 16, 17)
#else
  #define TEL        Serial             
  #define TEL_BEGIN() do{ Serial.begin(115200); Serial.setTxBufferSize(2048); }while(0)
#endif

static TaskHandle_t th = nullptr;

static inline void tx_line(const TelePkt& p){
  if (TEL.availableForWrite() < 128) return;   
  char buf[160];
  int n = snprintf(buf, sizeof(buf),
    "T,%u,TH,%d,SP,%d,E,%d,THD,%d,U,%d,P,%d,I,%d,D,%d,DUTY,%d,SAT,%u,KICK,%u,DT,%u\n",
    p.t_ms, p.th_c2, p.sp_c2, p.e_c2, p.thd_c2, p.u_c2, p.p_c2, p.i_c2, p.d_c2,
    p.duty, p.sat, p.kick, p.dt_ms);
  if (n > 0) TEL.write((const uint8_t*)buf, (size_t)n);
}

static void serial_tx_task(void*){
  TEL_BEGIN();
  TelePkt pkt{};
  for(;;){
    if (xQueueReceive(qTel, &pkt, pdMS_TO_TICKS(10)) == pdTRUE) {
      tx_line(pkt);
    }
  }
}

void serial_tx_task_start(){
  xTaskCreatePinnedToCore(serial_tx_task, "thTEL", 3072, nullptr,
                          tskIDLE_PRIORITY+1, &th, CORE_IO);
}
