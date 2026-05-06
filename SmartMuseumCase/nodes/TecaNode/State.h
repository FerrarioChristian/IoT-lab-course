#ifndef STATE_H
#define STATE_H

// ==========================================
// MACCHINA A STATI: STATO DEL SISTEMA
// ==========================================
enum SystemState { ARMED, DISARMED, ALARM_ACTIVE };

extern volatile SystemState currentState;

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

#endif // STATE_H