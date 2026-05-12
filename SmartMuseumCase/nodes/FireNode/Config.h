#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PIN MAPPING (NodeMCU ESP8266 V1) - FIRE NODE
// ==========================================

// INPUT
#define PIN_FLAME_A A0 // Analog (ADC0): Flame sensor analog output
#define PIN_FLAME_D D1 // D1 (GPIO5): Flame sensor digital output (Supporta Interrupt)

// ==========================================
// THRESHOLDS & SETTINGS
// ==========================================
extern int thresh_flame_analog_max; // Soglia analogica modificabile

// ==========================================
// MQTT & WOT SETTINGS
// ==========================================
#define MQTT_BROKERIP "149.132.176.75"
#define MQTT_USERNAME "ChristianFerrario"
#define MQTT_PASSWORD "iot886230"

#define WOT_DISCOVERY_TOPIC "christianferrario/museum/discovery"
#define MQTT_BASE_TOPIC "christianferrario/museum/"

#endif // CONFIG_H
