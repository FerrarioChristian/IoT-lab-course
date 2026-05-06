#ifndef STATE_H
#define STATE_H

// ==========================================
// MACCHINA A STATI: STATO DEL SISTEMA
// ==========================================
enum SystemState { ARMED, DISARMED, ALARM_ACTIVE };

extern volatile SystemState currentState;

// ==========================================
// STATO DELL'UI: PAGINE DISPLAY LCD
// ==========================================
enum PageState {
  PAGE_ENV,      // 1. Temp / Hum / Light
  PAGE_DISTANCE, // 2. Distanza / Stato sistema
  PAGE_WIFI      // 3. Info Rete: WiFi / IP
};

extern volatile PageState currentPage;

// ==========================================
// DATI SENSORI (Ricevuti via MQTT in futuro)
// ==========================================
struct SensorData {
  float temperature;
  float humidity;
  int lightLevel;
  float distanceCm;
  bool knockDetected;
  int wifiRssi;
};

extern volatile SensorData currentData;

// ==========================================
// FLAG PER INTERRUPTS ENCODER
// ==========================================
extern volatile bool flagEncoderPressed;
extern volatile int encoderCount;

#endif // STATE_H