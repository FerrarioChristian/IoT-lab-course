#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

// ==========================================
// MACCHINA A STATI: STATO DEL SISTEMA
// ==========================================
enum SystemState { ARMED = 0, DISARMED = 1, ALARM_ACTIVE = 2 };

extern volatile SystemState currentState;

// ==========================================
// STATO DELL'UI: PAGINE DISPLAY LCD
// ==========================================
enum PageState {
  STATE_OVERVIEW,
  STATE_NODE_LIST,
  STATE_NODE_DETAIL_1,
  STATE_NODE_DETAIL_2
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
  int flameAnalog;
};

// ==========================================
// REGISTRO DEI NODI (Multi-Node Master)
// ==========================================
#define MAX_NODES 10

struct Thresholds {
  float tempMax;
  float humMax;
  int lightMax;
  int distMin;
};

struct NodeCapabilities {
  bool hasTemperature;
  bool hasHumidity;
  bool hasLight;
  bool hasDistance;
  bool hasFlame;
  bool hasImpactEvent;
  bool hasFireEvent;
  bool hasWarningEvent;
};

struct NodeState {
  String id;
  SensorData data;
  SystemState state;
  bool activeWarning;
  unsigned long lastSeen;
  Thresholds settings;
  String alarmReason;
  NodeCapabilities capabilities;
};

extern NodeState nodeRegistry[MAX_NODES];
extern int activeNodeCount;

// Indice del nodo attualmente visualizzato/selezionato nel menu
extern int selectedNodeIndex;

// ==========================================
// FLAG PER INTERRUPTS ENCODER
// ==========================================
extern volatile bool flagEncoderPressed;
extern volatile int encoderCount;

#endif // STATE_H