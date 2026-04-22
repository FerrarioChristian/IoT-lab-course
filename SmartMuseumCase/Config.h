#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> 

// ==========================================
// PIN MAPPING (NodeMCU ESP8266 V1)
// ==========================================

// INPUT
#define PIN_LDR A0     // Analog (ADC0): Photo Resistor
#define PIN_ENC_CLK D3 // D3 (GPIO0) : Rotary Encoder CLK
#define PIN_ENC_DT D4  // D4 (GPIO2) : Rotary Encoder DT
#define PIN_DHT D5     // D5 (GPIO14): DHT11 (Temp/Hum)
#define PIN_KNOCK D6   // D6 (GPIO12): Hit/Knock Sensor (Digitale con interrupt)
#define PIN_HC_ECHO D7 // D7 (GPIO13): Ultrasound HC-SR04 ECHO
#define PIN_ENC_SW 3  // RX (GPIO3) : Rotary Encoder SW

// OUTPUT
#define PIN_HC_TRIG D0 // D0 (GPIO16): Ultrasound HC-SR04 TRIGGER
#define PIN_I2C_SCL D1 // D1 (GPIO5): I2C SCL per LCD
#define PIN_I2C_SDA D2 // D2 (GPIO4): I2C SDA per LCD

#define PIN_LED D8     // D8 (GPIO15): Indicator LED (Deve essere LOW al boot)
#define PIN_BUZZER 1   // TX (GPIO1): Passive Buzzer

// ==========================================
// THRESHOLDS & SETTINGS
// ==========================================
#define DHT_TYPE DHT11

// Variabili soglia di default, modificabili dalla dashboard
extern float thresh_temp_max;
extern float thresh_hum_max;
extern int thresh_distance_min;
extern int thresh_light_max;

// ==========================================
// WIFI & INFLUXDB CREDENTIALS
// ==========================================
#define WIFI_SSID "IoTLabThingsU14"
#define WIFI_PASSWORD "L@b%I0T*Ui4!P@sS**0%Lessons!"

#define INFLUXDB_URL "http://149.132.176.75:8086"
#define INFLUXDB_TOKEN "ag41x-d1pY2wDpaIs5E5w0szBh-ESTkPmE32VUK6zN4WgI-SvXM7_iMFbble9r7N3CCqQSkJdY0OHl7aGpNSVw=="
#define INFLUXDB_ORG "labiot-org"                   
#define INFLUXDB_BUCKET "ChristianFerrario-bucket"


#endif // CONFIG_H
