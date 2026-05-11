#include "Config.h"
#include "Sensors.h"
#include "State.h"
#include "WebInterface.h"

#include "NetworkManager.h"
#include "MqttManager.h"

// ==========================================
// ISTANZIAZIONE VARIABILI GLOBALI
// ==========================================
volatile SystemState currentState = ARMED;

volatile SensorData currentData = {0.0, 0.0, 0, 0.0, false, 0};

volatile bool flagKnockDetected = false;

// Valori predefiniti delle soglie (aggiornabili da WEB)
float thresh_temp_max = 30.0;
float thresh_hum_max = 60.0;
int thresh_distance_min = 10;
int thresh_light_max = 999;

NetworkManager networkManager;
MqttManager mqttManager(MQTT_BROKERIP, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

unsigned long lastTelemetryPublish = 0;
const unsigned long telemetryInterval = 5000; // Invia dati ogni 5 secondi

// ==========================================
// THING DESCRIPTION (WoT) JSON
// ==========================================
const char* thingDescription = R"rawliteral(
{
  "@context": "https://www.w3.org/2022/wot/td/v1.1",
  "id": "urn:dev:mac:teca_01",
  "title": "Smart Museum Display Case 01",
  "securityDefinitions": { "nosec_sc": { "scheme": "nosec" } },
  "security": "nosec_sc",
  "properties": {
    "telemetry": {
      "description": "Sensors telemetry: temperature, humidity, light",
      "forms": [{"href": "mqtt://149.132.176.75/christianferrario/museum/teca01/telemetry"}]
    }
  },
  "events": {
    "impactDetected": {
      "description": "Fired when the knock sensor detects an impact.",
      "data": {"type": "boolean"},
      "forms": [{"href": "mqtt://149.132.176.75/christianferrario/museum/teca01/events/impact"}]
    },
    "visitorWarning": {
      "description": "Fired when a visitor gets too close to the display case.",
      "data": {"type": "boolean"},
      "forms": [{"href": "mqtt://149.132.176.75/christianferrario/museum/teca01/events/warning"}]
    }
  }
}
)rawliteral";

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting Smart Museum Display Case (Teca Node)...");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  networkManager.connect("Teca-Setup-01");

  // Configura LWT (Last Will and Testament)
  mqttManager.setWill(MQTT_STATUS_TOPIC, "offline", true, 1);
  mqttManager.setup();

  // Pubblica lo stato e la Thing Description se connesso
  if (mqttManager.isConnected()) {
    mqttManager.publish(MQTT_STATUS_TOPIC, "online", true, 1);
    mqttManager.publish(WOT_DISCOVERY_TOPIC, thingDescription, true, 1);
  }

  setupSensors();
  setupWeb();

  addLog("==== SMART MUSEUM BOOT ====");
  addLog("Hardware NodeMCU inizializzato con successo (Teca Node).");
  addLog("Sensori pronti all'uso.");

  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  taskSensori();
  handleWebTask();
  mqttManager.loop();

  // Pubblica la telemetria ciclicamente via MQTT
  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryPublish >= telemetryInterval) {
    lastTelemetryPublish = currentMillis;
    
    String payload = "{";
    payload += "\"temperature\":" + String(currentData.temperature, 1) + ",";
    payload += "\"humidity\":" + String(currentData.humidity, 1) + ",";
    payload += "\"lightLevel\":" + String(currentData.lightLevel);
    payload += "}";

    Serial.println("Publishing telemetry: " + payload);
    mqttManager.publish(MQTT_TELEMETRY_TOPIC, payload.c_str());
  }

  // Controllo distanza (Event-Driven) con Debounce
  static bool visitorTooClose = false;
  static bool lastCondition = false;
  static unsigned long lastDistanceChangeTime = 0;
  const unsigned long distanceDebounceDelay = 1000; // 1 secondo di stabilità richiesta
  
  bool currentCondition = (currentData.distanceCm < thresh_distance_min);
  
  // Se la condizione grezza cambia (es. da lontano a vicino), resettiamo il timer
  if (currentCondition != lastCondition) {
    lastDistanceChangeTime = currentMillis;
    lastCondition = currentCondition;
  }
  
  // Se la condizione è rimasta stabile per il tempo di debounce
  if ((currentMillis - lastDistanceChangeTime) > distanceDebounceDelay) {
    if (currentCondition != visitorTooClose) {
      visitorTooClose = currentCondition;
      if (visitorTooClose) {
        addLog("WARNING: Visitor too close.");
        mqttManager.publish(MQTT_EVENT_WARNING_TOPIC, "true");
      } else {
        addLog("INFO: Visitor stepped back.");
        mqttManager.publish(MQTT_EVENT_WARNING_TOPIC, "false");
      }
    }
  }

  // Controllo interrupt di Allarme (Knock Sensor)
  if (flagKnockDetected) {
    flagKnockDetected = false;
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      addLog("INTRUSION: Knock sensor hardware triggered.");
      
      // Pubblica l'evento critico via MQTT immediatamente
      mqttManager.publish(MQTT_EVENT_IMPACT_TOPIC, "true");
    }
  }
}
