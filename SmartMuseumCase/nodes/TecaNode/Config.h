#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PIN MAPPING (NodeMCU ESP8266 V1) - TECA NODE
// ==========================================

// INPUT
#define PIN_LDR A0     // Analog (ADC0): Photo Resistor
#define PIN_DHT D5     // D5 (GPIO14): DHT11 (Temp/Hum)
#define PIN_KNOCK D6   // D6 (GPIO12): Hit/Knock Sensor (Digitale con interrupt)
#define PIN_HC_ECHO D7 // D7 (GPIO13): Ultrasound HC-SR04 ECHO

// OUTPUT
#define PIN_HC_TRIG D0 // D0 (GPIO16): Ultrasound HC-SR04 TRIGGER
#define PIN_LED D8     // D8 (GPIO15): Indicator LED

// ==========================================
// THRESHOLDS & SETTINGS
// ==========================================
#define DHT_TYPE DHT11

// Variabili modificabili (ora via MQTT dal Master)
extern float thresh_temp_max;
extern float thresh_hum_max;
extern int thresh_distance_min;
extern int thresh_light_max;

// ==========================================
// MQTT & WOT SETTINGS
// ==========================================
#define MQTT_BROKERIP "149.132.176.75"
#define MQTT_USERNAME "ChristianFerrario"
#define MQTT_PASSWORD "iot886230"

#define WOT_DISCOVERY_TOPIC "christianferrario/museum/discovery"
#define MQTT_BASE_TOPIC "christianferrario/museum/"

#endif // CONFIG_H