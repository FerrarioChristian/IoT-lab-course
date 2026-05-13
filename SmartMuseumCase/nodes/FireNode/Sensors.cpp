#include "Sensors.h"

volatile bool flagFireDetected = false;

ICACHE_RAM_ATTR void handleFlameInterrupt() {
  // Attivazione istantanea
  if (digitalRead(PIN_FLAME_D) == HIGH) { // Modifica in LOW se il tuo sensore è attivo basso
    flagFireDetected = true;
  }
}

void setupSensors() {
  pinMode(PIN_FLAME_A, INPUT);
  pinMode(PIN_FLAME_D, INPUT); // Modifica in INPUT_PULLUP se il sensore lo richiede
  
  // Attach interrupt per transizioni in salita
  attachInterrupt(digitalPinToInterrupt(PIN_FLAME_D), handleFlameInterrupt, RISING); // usa FALLING se attivo basso
}

void taskSensori() {
  // Vuoto, ora funziona tutto in modalità pura event-driven via interrupt
}
