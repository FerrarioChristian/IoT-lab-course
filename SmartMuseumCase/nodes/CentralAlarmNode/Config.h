#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PIN MAPPING (NodeMCU ESP8266 V1) - ALARM NODE
// ==========================================

// INPUT
#define PIN_ENC_CLK D3 // D3 (GPIO0) : Rotary Encoder CLK
#define PIN_ENC_DT D4  // D4 (GPIO2) : Rotary Encoder DT
#define PIN_ENC_SW D5  // D5 (GPIO14): Rotary Encoder SW (Spostato da RX)

// OUTPUT
#define PIN_I2C_SCL D1 // D1 (GPIO5): I2C SCL per LCD
#define PIN_I2C_SDA D2 // D2 (GPIO4): I2C SDA per LCD

#define PIN_BUZZER D6  // D6 (GPIO12): Passive Buzzer (Spostato da TX)

// RGB LED (PWM)
#define PIN_LED_R D7   // D7 (GPIO13)
#define PIN_LED_G D8   // D8 (GPIO15)
#define PIN_LED_B D0   // D0 (GPIO16)

// ==========================================
// THRESHOLDS & SETTINGS
// ==========================================
// Variabili modificabili (limiti globali o allarmi)
extern float thresh_temp_max;
extern float thresh_hum_max;
extern int thresh_distance_min;
extern int thresh_light_max;

// ==========================================
// MQTT & WOT SETTINGS
// ==========================================
#define MQTT_BROKERIP "149.132.176.75"
#define MQTT_CLIENTID "mqttx_886230node_alarm"
#define MQTT_USERNAME "ChristianFerrario"
#define MQTT_PASSWORD "iot886230"

#define WOT_DISCOVERY_TOPIC "christianferrario/museum/discovery"
#define MQTT_BASE_TOPIC "christianferrario/museum/"
#define MQTT_STATUS_TOPIC "christianferrario/museum/alarm01/status"

// Wildcard subscriptions for tracking all teca nodes
#define MQTT_WILDCARD_TELEMETRY "christianferrario/museum/+/telemetry"
#define MQTT_WILDCARD_IMPACT "christianferrario/museum/+/events/impact"
#define MQTT_WILDCARD_FIRE "christianferrario/museum/+/events/fire"
#define MQTT_WILDCARD_WARNING "christianferrario/museum/+/events/warning"

#endif // CONFIG_H