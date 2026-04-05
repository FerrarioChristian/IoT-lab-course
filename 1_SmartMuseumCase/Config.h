#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// PIN MAPPING (NodeMCU ESP8266 V1)
// ==========================================
#define PIN_LDR A0     // Analog (ADC0): Photo Resistor
#define PIN_I2C_SDA D2 // D2 (GPIO4): I2C SDA per LCD
#define PIN_I2C_SCL D1 // D1 (GPIO5): I2C SCL per LCD
#define PIN_DHT D5     // D5 (GPIO14): DHT11 (Temp/Hum)
#define PIN_HC_TRIG D0 // D0 (GPIO16): Ultrasound HC-SR04 TRIGGER
#define PIN_HC_ECHO D7 // D7 (GPIO13): Ultrasound HC-SR04 ECHO
#define PIN_KNOCK D6   // D6 (GPIO12): Hit/Knock Sensor (Digitale con interrupt)
#define PIN_ENC_CLK D3 // D3 (GPIO0) : Rotary Encoder CLK
#define PIN_ENC_DT D4  // D4 (GPIO2) : Rotary Encoder DT
#define PIN_ENC_SW RX  // RX (GPIO3) : Rotary Encoder SW
#define PIN_LED_RED D8 // D8 (GPIO15): Red LED (Deve essere LOW al boot)
#define PIN_LED_GREEN TX // TX (GPIO1) : Green LED

// ==========================================
// THRESHOLDS & SETTINGS
// ==========================================
#define DHT_TYPE DHT11

// Variabili soglia di default, modificabili dalla dashboard
extern float thresh_temp_max;
extern float thresh_hum_max;
extern int thresh_distance_min;
extern int thresh_light_min;

// ==========================================
// WIFI & INFLUXDB CREDENTIALS
// ==========================================
#define WIFI_SSID "IlTuoSSID"
#define WIFI_PASSWORD "LaTuaPassword"

#define INFLUXDB_URL "http://192.168.1.100:8086"
#define INFLUXDB_TOKEN "IlTuoTokenInluxDB"
#define INFLUXDB_ORG "museum_org"
#define INFLUXDB_BUCKET "museum_telemetry"

#endif // CONFIG_H
