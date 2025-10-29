#include <Arduino.h>

const int IN1 = 19;
const int IN2 = 18;
const int ENA = 25;

const int ENCA = 34;
const int ENCB = 35;

volatile int32_t enc_count = 0;
volatile uint8_t enc_last  = 0;

const int pwmFreq = 1000;
const int pwmRes  = 8;
const int pwmCh   = 0;
const int DUTY_MAX = (1 << pwmRes) - 1;

const float CPR  = 2400.0f;
const float GEAR = 1.0f;
const float TAU  = 2.0f * (float)PI;

float theta_deg = 0.0f;
float zero_offset = 0.0f;

float kp = 290.0f;
float ki = 0.0f;
float kd = 50.0f;

float i_term = 0.0f;
float prev_err = 0.0f;
unsigned long prev_ms = 0;
const float setpoint_deg = 0.0f;
const float i_band_deg = 8.0f;
const float u_sat = 60.0f;

const uint8_t DEADBAND_DUTY = 3;
const uint8_t KICK_DUTY = 18;
const uint16_t KICK_MS = 35;

IRAM_ATTR void enc_isr() {
  uint8_t a = digitalRead(ENCA);
  uint8_t b = digitalRead(ENCB);
  uint8_t now = (a << 1) | b;
  uint8_t idx = (enc_last << 2) | now;
  static const int8_t tab[16] = {0,-1,+1,0,+1,0,0,-1,-1,0,0,+1,0,+1,-1,0};
  enc_count += tab[idx];
  enc_last = now;
}

float read_angle_rad() {
  return ((float)enc_count / (CPR * GEAR)) * TAU;
}

float wrapPi(float x) {
  while (x >  PI) x -= TAU;
  while (x < -PI) x += TAU;
  return x;
}

void motorWriteSigned(int16_t dutySigned, bool kick) {
  if (dutySigned == 0) {
    ledcWrite(pwmCh, 0);
    return;
  }
  if (dutySigned > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  uint16_t mag = abs(dutySigned);
  if (mag > 0 && mag < DEADBAND_DUTY) mag = DEADBAND_DUTY;
  if (mag > DUTY_MAX) mag = DUTY_MAX;
  if (kick && mag == DEADBAND_DUTY) {
    ledcWrite(pwmCh, KICK_DUTY);
    delay(KICK_MS);
  }
  ledcWrite(pwmCh, mag);
}

void calibrate_zero() {
  Serial.println("\n==============================");
  Serial.println("🧭 Calibração automática de zero");
  Serial.println("Coloque o pêndulo na posição vertical.");
  Serial.println("A calibração começará em 10 segundos...");
  Serial.println("==============================");

  for (int i = 10; i > 0; i--) {
    Serial.printf("Iniciando em %d...\n", i);
    delay(1000);
  }

  zero_offset = wrapPi(read_angle_rad());
  Serial.printf("✅ Zero definido automaticamente: %.2f°\n\n",
                zero_offset * 180.0f / (float)PI);
}




void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcSetup(pwmCh, pwmFreq, pwmRes);
  ledcAttachPin(ENA, pwmCh);

  pinMode(ENCA, INPUT);
  pinMode(ENCB, INPUT);
  enc_last = (digitalRead(ENCA) << 1) | digitalRead(ENCB);
  attachInterrupt(digitalPinToInterrupt(ENCA), enc_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCB), enc_isr, CHANGE);

  delay(500);
  calibrate_zero();
  prev_ms = millis();
  Serial.println("PID PENDULO INVERTIDO");
  Serial.println("kp ki kd u_sat");
  Serial.printf("%.3f %.3f %.3f %.1f\n", kp, ki, kd, u_sat);
}

void loop() {
  unsigned long now = millis();
  float dt = (now - prev_ms) * 1e-3f;
  if (dt <= 0.0f) dt = 1e-3f;
  prev_ms = now;

  float theta = wrapPi(read_angle_rad() - zero_offset);
  theta_deg = theta * 180.0f / (float)PI;

  float err = setpoint_deg - theta_deg;

  if (fabsf(err) < i_band_deg) {
    i_term += ki * err * dt;
  } else {
    i_term = 0.0f;
  }

  float d_term = kd * (err - prev_err) / dt;
  float p_term = kp * err;

  float u = p_term + i_term + d_term;

  if (u >  u_sat) u =  u_sat;
  if (u < -u_sat) u = -u_sat;

  float mag = fabsf(u) / u_sat;
  if (mag < 0.0f) mag = 0.0f;
  if (mag > 1.0f) mag = 1.0f;

  int16_t duty = (int16_t)roundf(mag * DUTY_MAX);
  if (fabsf(err) < 0.8f && fabsf(u) < (0.10f * u_sat)) duty = 0;

  bool kick = (duty != 0);
  motorWriteSigned((u >= 0.0f) ? duty : -duty, kick);

  Serial.printf("th=%.2f deg | e=%.2f | P=%.2f I=%.2f D=%.2f | u=%.2f | duty=%d\n",
                theta_deg, err, p_term, i_term, d_term, u, (int)duty);

  prev_err = err;
}
