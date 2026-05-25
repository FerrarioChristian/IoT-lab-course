#include "Sensors.h"

volatile bool flagFireDetected = false;

ICACHE_RAM_ATTR void handleFlameInterrupt() {
  if (digitalRead(PIN_FLAME_D) == HIGH) {
    flagFireDetected = true;
  }
}

void setupSensors() {
  pinMode(PIN_FLAME_A, INPUT);
  pinMode(PIN_FLAME_D, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_FLAME_D), handleFlameInterrupt,
                  RISING);
}

void taskSensori() {
  // Vuoto, ora funziona tutto in modalità interrupt
}
