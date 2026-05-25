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

NetworkManager networkManager;
MqttManager* mqttManager;

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
  topicFire = baseTopic + "events/fire"; 
  topicStatus = baseTopic + "status";
  topicCmd = baseTopic + "actions/cmd";

  // Generazione Thing Description
  thingDescription = R"json({
  "@context": "https://www.w3.org/2022/wot/td/v1.1",
  "id": "urn:dev:mac:{{NODE_ID}}",
  "title": "Smart Museum Fire Node ({{NODE_ID}})",
  "securityDefinitions": { "nosec_sc": { "scheme": "nosec" } },
  "security": "nosec_sc",
  "properties": {
    "flameAnalog": {"type": "integer"}
  },
  "events": {
    "fireDetected": {
      "description": "Triggered when fire is detected",
      "data": {"type": "boolean"},
      "forms": [{"href": "mqtt://{{BROKER_IP}}/{{TOPIC_FIRE}}"}]
    }
  },
  "actions": {
    "cmd": {
      "description": "Send ARM, DISARM, or MUTE",
      "forms": [{"href": "mqtt://{{BROKER_IP}}/{{TOPIC_CMD}}"}]
    }
  }
})json";

  thingDescription.replace("{{NODE_ID}}", nodeId);
  thingDescription.replace("{{BROKER_IP}}", String(MQTT_BROKERIP));
  thingDescription.replace("{{TOPIC_FIRE}}", topicFire);
  thingDescription.replace("{{TOPIC_CMD}}", topicCmd);

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

  // ==========================================
  // EVENT-DRIVEN LOGIC: FIRE ALARM
  // ==========================================
  // L'allarme scatta istantaneamente flaggato dall'interrupt
  if (flagFireDetected) {
    flagFireDetected = false; // Reset flag
    
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      addLog("FIRE EMERGENCY: Flame detected via Hardware Interrupt!");
      
      // Invio messaggio istantaneo MQTT con QoS 1 per affidabilità
      mqttManager->publish(topicFire.c_str(), "true", false, 1);
    }
  }
}
