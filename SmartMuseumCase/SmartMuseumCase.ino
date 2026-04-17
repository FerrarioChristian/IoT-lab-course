/**
 * Smart Museum Display Case Monitoring System
 */

#include "Config.h"
#include "Database.h"
#include "Sensors.h"
#include "State.h"
#include "UI.h"
#include "WebInterface.h"

// ==========================================
// ISTANZIAZIONE VARIABILI GLOBALI (da State.h e Config.h)
// ==========================================
volatile SystemState currentState = ARMED;
volatile PageState currentPage = PAGE_WIFI;

volatile SensorData currentData = {0.0, 0.0, 0, 0.0, false, 0};

volatile bool flagKnockDetected = false;
volatile bool flagEncoderPressed = false;
volatile int encoderCount = 0;

// Valori predefiniti delle soglie (aggiornabili da WEB)
float thresh_temp_max = 25.0;
float thresh_hum_max = 60.0;
int thresh_distance_min = 10;
int thresh_light_max = 930;

// ==========================================
// SETUP
// ==========================================
void setup() {
  // TX(1) e RX(3) sono usati, quindi non é possibile usare il Serial Monitor
  // Serial.begin(115200);
  // delay(500);
  // Serial.println("\nBooting Smart Museum Display Case...");
  // delay(10);
  // Serial.end();

  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW); // Obbligatorio LOW al boot per D8(GPIO15)

  setupSensors();
  setupUI();
  setupWeb();
  setupDatabase();

  addLog("==== SMART MUSEUM BOOT ====");
  addLog("Hardware NodeMCU inizializzato con successo.");
  addLog("Database InfluxDB e moduli pronti all'uso.");

  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  taskSensori();
  taskUI();
  taskDatabase();
  handleWebTask();

  // Controllo immediato interrupt di Allarme (Knock Sensor)
  if (flagKnockDetected) {
    flagKnockDetected = false;
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      logSystemEvent("INTRUSION", "Knock sensor hardware triggered.");
      taskUI();
    } // Aggiorna UI immediatamente
  }

  // Controllo immediato bottone encoder
  if (flagEncoderPressed) {
    flagEncoderPressed = false;
    if (currentState == ALARM_ACTIVE) {
      currentState = ARMED; // Muta allarme e resetta lo stato
      logSystemEvent("ALARM_ACK", "Silenced by local hardware button.");
    }
  }
}
