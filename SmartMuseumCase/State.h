#ifndef STATE_H
#define STATE_H

// ==========================================
// MACCHINA A STATI: STATO DEL SISTEMA
// ==========================================
enum SystemState {
  ARMED, // Sistema attivo, monitora pre-allarmi (distanza) e allarmi (impatto)
  DISARMED, // Sistema disattivo, non scatta alcun allarme, raccolta telemetria
            // base
  ALARM_ACTIVE // Allarme scattato per impatto, LED rosso fisso/buzzer. Richiede
               // blocco.
};

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
// DATI SENSORI
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
// FLAG PER INTERRUPTS
// ==========================================
extern volatile bool flagKnockDetected;
extern volatile bool flagEncoderPressed;
extern volatile int encoderCount;

#endif // STATE_H
