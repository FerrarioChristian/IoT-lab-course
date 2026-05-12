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
volatile SensorData currentData = {0, false};

// Valori predefiniti delle soglie
int thresh_flame_analog_max = 500;

NetworkManager networkManager;
MqttManager* mqttManager;

unsigned long lastTelemetryPublish = 0;
const unsigned long telemetryInterval = 5000;

// Variabili dinamiche
String nodeId;
String clientId;
String topicTelemetry;
String topicFire; 
String topicStatus;
String topicCmd;
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
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting Smart Museum Fire Node...");

  networkManager.connect("Fire-Setup");

  // Generazione ID dinamico basato sul MAC Address
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  nodeId = "fire_" + mac.substring(6);
  clientId = "mqttx_" + nodeId;

  // Generazione Topic dinamici
  String baseTopic = String(MQTT_BASE_TOPIC) + nodeId + "/";
  topicTelemetry = baseTopic + "telemetry";
  // Usiamo topicFire per un allarme incendio specifico
  topicFire = baseTopic + "events/fire"; 
  topicStatus = baseTopic + "status";
  topicCmd = baseTopic + "actions/cmd";

  // Generazione Thing Description
  thingDescription = "{\n";
  thingDescription += "  \"@context\": \"https://www.w3.org/2022/wot/td/v1.1\",\n";
  thingDescription += "  \"id\": \"urn:dev:mac:" + nodeId + "\",\n";
  thingDescription += "  \"title\": \"Smart Museum Fire Node (" + nodeId + ")\",\n";
  thingDescription += "  \"securityDefinitions\": { \"nosec_sc\": { \"scheme\": \"nosec\" } },\n";
  thingDescription += "  \"security\": \"nosec_sc\",\n";
  thingDescription += "  \"properties\": {\n";
  thingDescription += "    \"telemetry\": {\n";
  thingDescription += "      \"description\": \"Flame sensor telemetry\",\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicTelemetry + "\"}]\n";
  thingDescription += "    }\n";
  thingDescription += "  },\n";
  thingDescription += "  \"events\": {\n";
  thingDescription += "    \"fireDetected\": {\n";
  thingDescription += "      \"description\": \"Triggered when fire is detected\",\n";
  thingDescription += "      \"data\": {\"type\": \"boolean\"},\n";
  thingDescription += "      \"forms\": [{\"href\": \"mqtt://" + String(MQTT_BROKERIP) + "/" + topicFire + "\"}]\n";
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
  }

  setupSensors();

  addLog("==== FIRE NODE BOOT ====");
  addLog("Hardware NodeMCU inizializzato con successo (" + nodeId + ").");
  addLog("Sensore fiamma KY-026 pronto all'uso.");

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
      wasConnected = true;
  } else if (!mqttManager->isConnected()) {
      wasConnected = false;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryPublish >= telemetryInterval) {
    lastTelemetryPublish = currentMillis;
    
    String payload = "{";
    payload += "\"flameAnalog\":" + String(currentData.flameAnalogLevel) + ",";
    payload += "\"flameDetected\":" + String(currentData.isFlameDetected ? "true" : "false") + ",";
    payload += "\"currentState\":" + String(currentState);
    payload += "}";

    // Serial.println("Publishing telemetry: " + payload);
    mqttManager->publish(topicTelemetry.c_str(), payload.c_str());
  }

  // ==========================================
  // EVENT-DRIVEN LOGIC: FIRE ALARM
  // ==========================================
  // Usa un semplice debouncer per evitare falsi allarmi
  static unsigned long lastFireDetectTime = 0;
  static bool lastFireState = false;
  
  if (currentData.isFlameDetected && currentState == ARMED) {
    if (!lastFireState) {
      lastFireDetectTime = currentMillis;
      lastFireState = true;
    }
    
    if (currentMillis - lastFireDetectTime > 500) { // 500ms di conferma continua
      currentState = ALARM_ACTIVE;
      addLog("FIRE EMERGENCY: Flame detected!");
      
      // Invio messaggio al nuovo topic specifico per il fuoco
      mqttManager->publish(topicFire.c_str(), "true");
    }
  } else {
    lastFireState = false;
  }
}
