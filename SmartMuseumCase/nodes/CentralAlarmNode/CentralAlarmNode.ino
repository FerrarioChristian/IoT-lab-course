#include "Config.h"
#include "State.h"
#include "Display.h"
#include "Actuators.h"

#include "NetworkManager.h"
#include "MqttManager.h"

// ==========================================
// ISTANZIAZIONE VARIABILI GLOBALI
// ==========================================
volatile SystemState currentState = ARMED;
volatile PageState currentPage = PAGE_WIFI;

// Dati ricevuti dalle teche per poterli visualizzare sul display
volatile SensorData currentData = {0.0, 0.0, 0, 0.0, false, 0};

volatile bool flagKnockDetected = false;
volatile bool flagEncoderPressed = false;
volatile int encoderCount = 0;

// Valori predefiniti delle soglie (aggiornabili)
float thresh_temp_max = 30.0;
float thresh_hum_max = 60.0;
int thresh_distance_min = 10;
int thresh_light_max = 999;

NetworkManager networkManager;
MqttManager mqttManager(MQTT_BROKERIP, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// ==========================================
// THING DESCRIPTION (WoT) JSON
// ==========================================
const char* thingDescription = R"rawliteral(
{
  "@context": "https://www.w3.org/2022/wot/td/v1.1",
  "id": "urn:dev:mac:alarm_01",
  "title": "Central Alarm Panel 01",
  "securityDefinitions": { "nosec_sc": { "scheme": "nosec" } },
  "security": "nosec_sc",
  "properties": {
    "status": {
      "type": "string",
      "forms": [{"href": "mqtt://149.132.176.75/christianferrario/museum/alarm01/status"}]
    }
  }
}
)rawliteral";

// ==========================================
// MQTT CALLBACK
// ==========================================
void onMqttMessage(String &topic, String &payload) {
  Serial.println("Ricevuto msg su " + topic + ": " + payload);
  
  if (topic == MQTT_EVENT_IMPACT_TOPIC) {
    if (payload == "true") {
      currentState = ALARM_ACTIVE;
      Serial.println("!!! ALLARME INTRUSIONE RICEVUTO !!!");
    }
  } else if (topic == MQTT_TELEMETRY_TOPIC) {
    // Parsing manuale del JSON per evitare dipendenze pesanti
    int tIdx = payload.indexOf("\"temperature\":");
    if (tIdx > 0) currentData.temperature = payload.substring(tIdx + 14, payload.indexOf(",", tIdx)).toFloat();
    
    int hIdx = payload.indexOf("\"humidity\":");
    if (hIdx > 0) currentData.humidity = payload.substring(hIdx + 11, payload.indexOf(",", hIdx)).toFloat();

    int lIdx = payload.indexOf("\"lightLevel\":");
    if (lIdx > 0) currentData.lightLevel = payload.substring(lIdx + 13, payload.indexOf(",", lIdx)).toInt();

    int dIdx = payload.indexOf("\"distanceCm\":");
    if (dIdx > 0) currentData.distanceCm = payload.substring(dIdx + 13, payload.indexOf("}", dIdx)).toFloat();
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting Central Alarm Node...");

  setupActuators();
  setupDisplay(); // Inizializza l'I2C e l'LCD subito
  
  // Mostra il messaggio a schermo prima di bloccare il processo nella ricerca Wi-Fi
  showSetupMessage("Alarm-Setup-01");

  networkManager.connect("Alarm-Setup-01");

  mqttManager.setWill(MQTT_STATUS_TOPIC, "offline", true, 1);
  mqttManager.setup();
  mqttManager.onMessage(onMqttMessage);

  if (mqttManager.isConnected()) {
    mqttManager.publish(MQTT_STATUS_TOPIC, "online", true, 1);
    mqttManager.publish(WOT_DISCOVERY_TOPIC, thingDescription, true, 1);
    
    // Iscrizione ai topic della teca
    mqttManager.subscribe(MQTT_TELEMETRY_TOPIC);
    mqttManager.subscribe(MQTT_EVENT_IMPACT_TOPIC);
    Serial.println("Iscritto ai topic della teca.");
  }

  Serial.println("==== CENTRAL ALARM BOOT ====");
  Serial.println("Hardware NodeMCU inizializzato con successo.");

  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  taskDisplay();
  mqttManager.loop();

  // Assicurati che le iscrizioni vengano ripristinate se il broker si disconnette e riconnette
  static bool wasConnected = true;
  if (mqttManager.isConnected() && !wasConnected) {
      mqttManager.subscribe(MQTT_TELEMETRY_TOPIC);
      mqttManager.subscribe(MQTT_EVENT_IMPACT_TOPIC);
      wasConnected = true;
  } else if (!mqttManager.isConnected()) {
      wasConnected = false;
  }

  // --- LOGICA ATTUATORI ---
  bool isAlarm = (currentState == ALARM_ACTIVE);
  bool isWarning =
      (currentState == ARMED && currentData.distanceCm > 0 && currentData.distanceCm < thresh_distance_min) ||
      (currentData.temperature > thresh_temp_max) ||
      (currentData.humidity > thresh_hum_max) ||
      (currentData.lightLevel > thresh_light_max);

  if (isAlarm) {
    playAlarm();
  } else if (isWarning) {
    playWarning();
  } else {
    stopActuators();
  }

  // Controllo bottone encoder
  if (flagEncoderPressed) {
    flagEncoderPressed = false;
    if (currentState == ALARM_ACTIVE) {
      currentState = ARMED; // Muta allarme
      Serial.println("ALARM_ACK: Silenced by local hardware button.");
    }
  }
}