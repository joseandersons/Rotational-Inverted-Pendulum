#include "encoder.h"
#include "config.h"

static volatile int32_t enc_count=0;
static volatile uint8_t enc_last=0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

IRAM_ATTR static void enc_isr(){
  uint8_t a=digitalRead(ENCA);
  uint8_t b=digitalRead(ENCB);
  uint8_t now=(a<<1)|b;
  uint8_t idx=(enc_last<<2)|now;
  static const int8_t tab[16]={0,-1,+1,0,+1,0,0,-1,-1,0,0,+1,0,+1,-1,0};
  portENTER_CRITICAL_ISR(&mux);
  enc_count+=tab[idx];
  portEXIT_CRITICAL_ISR(&mux);
  enc_last=now;
}

void encoder_isr_init(){
  pinMode(ENCA, INPUT);
  pinMode(ENCB, INPUT);
  enc_last=(digitalRead(ENCA)<<1)|digitalRead(ENCB);
  attachInterrupt(digitalPinToInterrupt(ENCA), enc_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCB), enc_isr, CHANGE);
}

int32_t encoder_get_count_isr_safe(){
  portENTER_CRITICAL(&mux);
  int32_t v=enc_count;
  portEXIT_CRITICAL(&mux);
  return v;
}

float encoder_get_angle_rad_now(){
  return ((float)encoder_get_count_isr_safe()/(CPR*GEAR))*TAU;
}
