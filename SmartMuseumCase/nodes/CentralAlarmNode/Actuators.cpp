#include "Actuators.h"
#include "Config.h"

static bool isBuzzing = false;
static unsigned int currentFreq = 0;

void setRGB(int r, int g, int b) {
  // Supponendo LED RGB a catodo comune.
  // Se è ad anodo comune, usa (255 - r) ecc.
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

void setupActuators() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setRGB(0, 255, 0); // Default: Verde (Sistema OK)
}

void playAlarm() {
  unsigned long currentMillis = millis();
  
  // Sirena bitonale
  unsigned int freq = ((currentMillis / 300) % 2 == 0) ? 1200 : 800;
  if (freq != currentFreq) {
    pinMode(PIN_BUZZER, OUTPUT);
    tone(PIN_BUZZER, freq);
    currentFreq = freq;
  }
  isBuzzing = true;

  // LED Rosso lampeggiante
  if ((currentMillis / 200) % 2 == 0) {
    setRGB(255, 0, 0);
  } else {
    setRGB(0, 0, 0);
  }
}

void playWarning() {
  unsigned long currentMillis = millis();

  // Beep intermittente
  if (currentMillis % 1000 < 150) {
    if (currentFreq != 1000) {
      pinMode(PIN_BUZZER, OUTPUT);
      tone(PIN_BUZZER, 1000);
      currentFreq = 1000;
    }
    isBuzzing = true;
  } else {
    if (isBuzzing) {
      noTone(PIN_BUZZER);
      pinMode(PIN_BUZZER, INPUT); // Scollega timer
      isBuzzing = false;
      currentFreq = 0;
    }
  }

  // LED Giallo
  setRGB(255, 128, 0);
}

void stopActuators() {
  if (isBuzzing || currentFreq != 0) {
    noTone(PIN_BUZZER);
    pinMode(PIN_BUZZER, INPUT);
    isBuzzing = false;
    currentFreq = 0;
  }
  // LED Verde Fisso
  setRGB(0, 255, 0);
}