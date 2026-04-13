/**
 * Smart Museum Display Case Monitoring System
 */

#include "Config.h"
#include "Database.h"
#include "Sensors.h"
#include "State.h"
#include "UI.h"
#include "WebInterface.h"
#include <Ticker.h>

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

// Oggetti Ticker per operazioni non bloccanti
Ticker sensorTicker;
Ticker uiTicker;
Ticker telemetryTicker;

void taskTelemetry() {
  // logDatabaseTask();
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  // NOTA HARDWARE: TX(1) e RX(3) sono usati per Encoder e LED.
  // Usarli per il Serial incasina i pin hardware, per cui se dobbiamo
  // chiamare Serial.begin(), bisogna fare molta attenzione, o disattivarlo.
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting Smart Museum Display Case...");
  delay(10);
  Serial.end();

  // Setup base dei PIN prima dell'assegnazione ai moduli specifici
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW); // Obbligatorio LOW al boot per D8(GPIO15)

  setupSensors();
  setupUI();
  setupWeb();
  setupDatabase();

  // Test di avvio per il nuovo Web Serial Monitor!
  addLog("==== SMART MUSEUM BOOT ====");
  addLog("Hardware NodeMCU inizializzato con successo.");
  addLog("Database InfluxDB e moduli pronti all'uso.");

  // Lo stato iniziale deve essere chiaramente armato e in sicurezza
  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  // Esecuzione continua tasks asincroni (internamente usano millis)
  taskSensori();
  taskUI();
  taskDatabase();
  handleWebTask(); // Chiamata al web server handleClient()

  // Controllo immediato interrupt di Allarme (Knock Sensor)
  if (flagKnockDetected) {
    flagKnockDetected = false;
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      logSystemEvent("INTRUSION", "Knock sensor hardware triggered.");
      taskUI();
    } // Aggiorna UI immediatamente
  }

  // Controllo immediato input dell'utente locale o sblocco (SW Bottone)
  if (flagEncoderPressed) {
    flagEncoderPressed = false;
    if (currentState == ALARM_ACTIVE) {
      currentState = ARMED; // Muta allarme e resetta lo stato (Acknowledge)
      logSystemEvent("ALARM_ACK", "Silenced by local hardware button.");
    }
  }
}
