#include "Sensors.h"

ICACHE_RAM_ATTR void handleFlameInterrupt() {
  // Leggiamo immediatamente il nuovo stato del pin
  currentData.isFlameDetected = (digitalRead(PIN_FLAME_D) == HIGH);
}

void setupSensors() {
  pinMode(PIN_FLAME_A, INPUT);
  pinMode(PIN_FLAME_D, INPUT); // Modifica in INPUT_PULLUP se il sensore lo richiede
  
  // Attach interrupt sul pin D1 (GPIO5) che supporta pienamente gli interrupt hardware
  attachInterrupt(digitalPinToInterrupt(PIN_FLAME_D), handleFlameInterrupt, CHANGE);
  
  // Inizializza lo stato al boot
  currentData.isFlameDetected = (digitalRead(PIN_FLAME_D) == HIGH);
}

void taskSensori() {
  // La lettura digitale e' ora gestita dall'interrupt in tempo reale.
  // Qui nel task periodico leggiamo solo il valore analogico per la telemetria.
  currentData.flameAnalogLevel = analogRead(PIN_FLAME_A);
}
