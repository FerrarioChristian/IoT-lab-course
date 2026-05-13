#include "Config.h"
#include "Sensors.h"
#include "State.h"

#include "NetworkManager.h"
#include "MqttManager.h"
#include <ESP8266WiFi.h>

// ==========================================
// ISTANZIAZIONE VARIABILI GLOBALI
// ==========================================
volatile SystemState currentState = ARMED;
volatile SensorData currentData = {0.0, 0.0, 0, 0.0, false, 0};
volatile bool flagKnockDetected = false;

// Valori predefiniti delle soglie (aggiornabili da MQTT)
float thresh_temp_max = 30.0;
float thresh_hum_max = 60.0;
int thresh_distance_min = 10;
int thresh_light_max = 999;

NetworkManager networkManager;
MqttManager* mqttManager; // Allocato dinamicamente in setup()

unsigned long lastTelemetryPublish = 0;
const unsigned long telemetryInterval = 5000;

// Variabili dinamiche
String nodeId;
String clientId;
String topicTelemetry;
String topicImpact;
String topicWarning;
String topicStatus;
String topicCmd;
String topicSettingsDist;
String thingDescription;

void addLog(String msg) {
  Serial.println(msg);
}

// ==========================================
// MQTT CALLBACK (Gestione Comandi)
// ==========================================
void onMqttMessage(String &topic, String &payload) {
  Serial.println("Comando ricevuto su " + topic + ": " + payload);
  
  if (topic == topicCmd) {
    if (payload == "ARM") {
      currentState = ARMED;
      addLog("SYSTEM_ARMED: Armed via MQTT.");
    } else if (payload == "DISARM") {
      currentState = DISARMED;
      addLog("SYSTEM_DISARMED: Disarmed via MQTT.");
    } else if (payload == "MUTE" && currentState == ALARM_ACTIVE) {
      currentState = ARMED;
      addLog("ALARM_ACK: Silenced via MQTT.");
    }
  } else if (topic == topicSettingsDist) {
    thresh_distance_min = payload.toInt();
    addLog("Soglia distanza aggiornata a: " + String(thresh_distance_min) + " cm");
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting Smart Museum Display Case (Teca Node)...");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  networkManager.connect("Teca-Setup");

  // Generazione ID dinamico basato sul MAC Address
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  nodeId = "teca_" + mac.substring(6); // Usa gli ultimi 6 caratteri
  clientId = "mqttx_" + nodeId;

  // Generazione Topic dinamici
  String baseTopic = String(MQTT_BASE_TOPIC) + nodeId + "/";
  topicTelemetry = baseTopic + "telemetry";
  topicImpact = baseTopic + "events/impact";
  topicWarning = baseTopic + "events/warning";
  topicStatus = baseTopic + "status";
  topicCmd = baseTopic + "actions/cmd";
  topicSettingsDist = baseTopic + "settings/distance";

  // Generazione Thing Description
  thingDescription = "{\n";
  thingDescription += "  \"@context\": \"https://www.w3.org/2022/wot/td/v1.1\",\n";
  thingDescription += "  \"id\": \"urn:dev:mac:" + nodeId + "\",\n";
  thingDescription += "  \"title\": \"Smart Museum Display Case (" + nodeId + ")\",\n";
  thingDescription += "  \"securityDefinitions\": { \"nosec_sc\": { \"scheme\": \"nosec\" } },\n";
  thingDescription += "  \"security\": \"nosec_sc\",\n";
  thingDescription += "  \"properties\": {\n";
  thingDescription += "    \"telemetry\": {\n";
  thingDescription += "      \"description\": \"Sensors telemetry\",\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicTelemetry + "\"}]\n";
  thingDescription += "    }\n";
  thingDescription += "  },\n";
  thingDescription += "  \"events\": {\n";
  thingDescription += "    \"impactDetected\": {\n";
  thingDescription += "      \"data\": {\"type\": \"boolean\"},\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicImpact + "\"}]\n";
  thingDescription += "    },\n";
  thingDescription += "    \"visitorWarning\": {\n";
  thingDescription += "      \"data\": {\"type\": \"boolean\"},\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicWarning + "\"}]\n";
  thingDescription += "    }\n";
  thingDescription += "  },\n";
  thingDescription += "  \"actions\": {\n";
  thingDescription += "    \"cmd\": {\n";
  thingDescription += "      \"description\": \"Send ARM, DISARM, or MUTE\",\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicCmd + "\"}]\n";
  thingDescription += "    }\n";
  thingDescription += "  }\n";
  thingDescription += "}";

  // Setup MQTT
  mqttManager = new MqttManager(MQTT_BROKERIP, clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
  mqttManager->setWill(topicStatus.c_str(), "offline", true, 1);
  mqttManager->setup();
  mqttManager->onMessage(onMqttMessage);

  if (mqttManager->isConnected()) {
    mqttManager->publish(topicStatus.c_str(), "online", true, 1);
    mqttManager->publish(WOT_DISCOVERY_TOPIC, thingDescription.c_str(), true, 1);
    mqttManager->subscribe(topicCmd.c_str());
    mqttManager->subscribe(topicSettingsDist.c_str());
  }

  setupSensors();

  addLog("==== SMART MUSEUM BOOT ====");
  addLog("Hardware NodeMCU inizializzato con successo (" + nodeId + ").");
  addLog("Sensori pronti all'uso.");

  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  taskSensori();
  mqttManager->loop();
  
  // Re-subscribe if reconnected
  static bool wasConnected = true;
  if (mqttManager->isConnected() && !wasConnected) {
      mqttManager->subscribe(topicCmd.c_str());
      mqttManager->subscribe(topicSettingsDist.c_str());
      wasConnected = true;
  } else if (!mqttManager->isConnected()) {
      wasConnected = false;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryPublish >= telemetryInterval) {
    lastTelemetryPublish = currentMillis;
    
    String payload = "{";
    payload += "\"temperature\":" + String(currentData.temperature, 1) + ",";
    payload += "\"humidity\":" + String(currentData.humidity, 1) + ",";
    payload += "\"lightLevel\":" + String(currentData.lightLevel) + ",";
    payload += "\"distanceCm\":" + String(currentData.distanceCm, 1) + ",";
    payload += "\"currentState\":" + String(currentState);
    payload += "}";

    Serial.println("Publishing telemetry: " + payload);
    mqttManager->publish(topicTelemetry.c_str(), payload.c_str());
  }

  // Controllo distanza (Event-Driven) con Debounce
  static bool visitorTooClose = false;
  static bool lastCondition = false;
  static unsigned long lastDistanceChangeTime = 0;
  const unsigned long distanceDebounceDelay = 1000;
  
  bool currentCondition = (currentData.distanceCm < thresh_distance_min);
  
  if (currentCondition != lastCondition) {
    lastDistanceChangeTime = currentMillis;
    lastCondition = currentCondition;
  }
  
  if ((currentMillis - lastDistanceChangeTime) > distanceDebounceDelay) {
    if (currentCondition != visitorTooClose) {
      visitorTooClose = currentCondition;
      if (visitorTooClose) {
        addLog("WARNING: Visitor too close.");
        mqttManager->publish(topicWarning.c_str(), "true");
      } else {
        addLog("INFO: Visitor stepped back.");
        mqttManager->publish(topicWarning.c_str(), "false");
      }
    }
  }

  // Controllo interrupt di Allarme (Knock Sensor)
  if (flagKnockDetected) {
    flagKnockDetected = false;
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      addLog("INTRUSION: Knock sensor hardware triggered.");
      mqttManager->publish(topicImpact.c_str(), "true");
    }
  }
}