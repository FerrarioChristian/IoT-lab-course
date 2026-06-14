#include "Actuators.h"
#include "Config.h"
#include <Ticker.h>

static bool isBuzzing = false;
static bool isWarning = false;

Ticker buzzerTicker;
Ticker ledTicker;

void setRGB(int r, int g, int b) {
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

void setupActuators() {
  // Inizializza il buzzer come INPUT (High-Impedance)
  pinMode(PIN_BUZZER, INPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setRGB(0, 255, 0); // Default: Verde
}

void alarmBuzzerTick() {
    static bool highFreq = true;
    if (highFreq) tone(PIN_BUZZER, 1200);
    else tone(PIN_BUZZER, 800);
    highFreq = !highFreq;
}

void alarmLedTick() {
    static bool ledOn = true;
    if (ledOn) setRGB(255, 0, 0);
    else setRGB(0, 0, 0);
    ledOn = !ledOn;
}

void warningBuzzerTick() {
    static bool beepOn = true;
    if (beepOn) tone(PIN_BUZZER, 1000);
    else noTone(PIN_BUZZER);
    beepOn = !beepOn;
}

void playAlarm() {
  if (isWarning) stopActuators();
  if (!isBuzzing) {
    pinMode(PIN_BUZZER, OUTPUT);
    buzzerTicker.attach_ms(300, alarmBuzzerTick);
    ledTicker.attach_ms(200, alarmLedTick);
    isBuzzing = true;
  }
}

void playWarning() {
  if (isBuzzing) stopActuators();
  if (!isWarning) {
    pinMode(PIN_BUZZER, OUTPUT);
    buzzerTicker.attach_ms(500, warningBuzzerTick);
    setRGB(255, 128, 0); // Giallo fisso
    isWarning = true;
  }
}

void stopActuators() {
  if (isBuzzing || isWarning) {
    buzzerTicker.detach();
    ledTicker.detach();
    noTone(PIN_BUZZER);
    pinMode(PIN_BUZZER, INPUT);
    setRGB(0, 255, 0);
    isBuzzing = false;
    isWarning = false;
  }
}