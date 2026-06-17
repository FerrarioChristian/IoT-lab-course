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
String topicDiscovery;
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
  } else if (topic == "christianferrario/museum/system/discovery_request") {
    addLog("Ricevuta richiesta di discovery dal Master, invio TD...");
    mqttManager->publish(topicDiscovery.c_str(), thingDescription.c_str());
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
  topicDiscovery = baseTopic + "discovery";

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
  // Setup MQTT
  mqttManager = new MqttManager(MQTT_BROKERIP, clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
  mqttManager->setWill(topicStatus.c_str(), "offline", true, 1);
  mqttManager->setup();
  mqttManager->onMessage(onMqttMessage);
  
  if (mqttManager->isConnected()) {
    mqttManager->publish(topicStatus.c_str(), "online", true, 1);
    mqttManager->publish(topicDiscovery.c_str(), thingDescription.c_str());
    mqttManager->subscribe(topicCmd.c_str());
    mqttManager->subscribe("christianferrario/museum/system/discovery_request");
    addLog("Inviato stato online e sottoscritto ai topic.");
  }
  
  // Aspetta che MQTT si stabilizzi e riceva eventuali messaggi (es. MUTE, DISARM, o Discovery)
  for(int i=0; i<50; i++) {
    mqttManager->loop();
    delay(10);
  }

  // --- LOGICA SENSORI ---
  setupSensors();
  
  // Lettura istantanea
  int analogVal = analogRead(PIN_FLAME_A);
  bool isFire = (digitalRead(PIN_FLAME_D) == HIGH);
  
  if (isFire || flagFireDetected) {
    if (currentState == ARMED) {
      currentState = ALARM_ACTIVE;
      addLog("FIRE DETECTED: Hardware sensor triggered.");
      mqttManager->publish(topicFire.c_str(), "true", false, 1);
    }
  }

  // --- PUBBLICAZIONE TELEMETRIA ---
  String payload = "{";
  payload += "\"flameAnalog\":" + String(analogVal) + ",";
  payload += "\"currentState\":" + String(currentState);
  payload += "}";

  Serial.println("Publishing telemetry: " + payload);
  mqttManager->publish(topicTelemetry.c_str(), payload.c_str());

  // Dai tempo ad MQTT di spedire i messaggi
  delay(100);

  // --- DEEP SLEEP ---
  Serial.println("Going into Deep Sleep for 5 seconds...");
  // Nota per il professore: il pin D0 deve essere collegato al pin RST!
  ESP.deepSleep(5e6); // 5 secondi = 5.000.000 microsecondi
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  // In modalità Deep Sleep il loop non viene mai raggiunto,
  // perché al termine del setup il microcontrollore si spegne.
  // Al termine del timer (5 secondi), l'impulso su RST lo riavvierà.
}
