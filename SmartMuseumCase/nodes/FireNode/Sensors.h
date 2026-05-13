#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Config.h"

extern volatile bool flagFireDetected;

void setupSensors();
void taskSensori();

#endif // SENSORS_H
