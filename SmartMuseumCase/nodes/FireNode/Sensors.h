#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Config.h"

// Struttura per contenere i dati del sensore di fiamma
struct SensorData {
  int flameAnalogLevel;
  bool isFlameDetected;
};

extern volatile SensorData currentData;

void setupSensors();
void taskSensori();

#endif // SENSORS_H
