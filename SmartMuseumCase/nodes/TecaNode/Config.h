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

// Variabili modificabili dalla dashboard (o via MQTT)
extern float thresh_temp_max;
extern float thresh_hum_max;
extern int thresh_distance_min;
extern int thresh_light_max;

// ==========================================
// WIFI CREDENTIALS
// ==========================================
// WiFi is now handled by WiFiManager (NetworkManager.h)

// ==========================================
// MQTT & WOT SETTINGS
// ==========================================
#define MQTT_BROKERIP "149.132.176.75"
#define MQTT_CLIENTID "mqttx_886230node_teca"
#define MQTT_USERNAME "ChristianFerrario"
#define MQTT_PASSWORD "iot886230"

#define WOT_DISCOVERY_TOPIC "christianferrario/museum/discovery"
#define MQTT_TELEMETRY_TOPIC "christianferrario/museum/teca01/telemetry"
#define MQTT_EVENT_IMPACT_TOPIC "christianferrario/museum/teca01/events/impact"
#define MQTT_STATUS_TOPIC "christianferrario/museum/teca01/status"

#endif // CONFIG_H
