#include <Arduino.h>
#include "serial_cmd.h"
#include "params.h"
#include "config.h"

static TaskHandle_t th = nullptr;

static void serial_cmd_task(void*){
  Serial.println("[serial] cmd task start");
  static char buf[96];
  size_t n = 0;

  for(;;){
    while(Serial.available()){
      char c = (char)Serial.read();
      if (c == '\n' || n >= sizeof(buf)-1){
        buf[n] = 0;

        if (n){
          float kp,ki,kd;
          if (sscanf(buf, "G %f %f %f", &kp, &ki, &kd) == 3){
            params_set(kp,ki,kd);
            auto g = params_get();
            Serial.printf("G %.6f %.6f %.6f\n", g.kp, g.ki, g.kd);
          } else if (strcmp(buf, "GET") == 0){
            auto g = params_get();
            Serial.printf("G %.6f %.6f %.6f\n", g.kp, g.ki, g.kd);
          } else if (strcmp(buf, "SAVE") == 0){
            params_save();
            Serial.println("OK");
          } else {
            Serial.println("ERR");
          }
        }
        n = 0;
      } else if (c != '\r'){
        buf[n++] = c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void serial_cmd_start(){
  xTaskCreatePinnedToCore(serial_cmd_task, "thSER", 3072, nullptr,
                          PRIO_ENCODER, &th, CORE_IO);
}
