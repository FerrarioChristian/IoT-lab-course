#include "Config.h"
#include "State.h"
#include "Display.h"
#include "Actuators.h"
#include "WebInterface.h"

#include "NetworkManager.h"
#include "MqttManager.h"
#include <ArduinoJson.h>

// ==========================================
// ISTANZIAZIONE VARIABILI GLOBALI
// ==========================================
volatile SystemState currentState = ARMED; // Stato globale dell'Allarme
volatile PageState currentPage = STATE_OVERVIEW;

NodeState nodeRegistry[MAX_NODES];
int activeNodeCount = 0;
int selectedNodeIndex = 0;

volatile bool flagKnockDetected = false;
volatile bool flagEncoderPressed = false;
volatile int encoderCount = 0;

// Valori predefiniti delle soglie (aggiornabili da Web)
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
  "id": "urn:dev:mac:central_alarm",
  "title": "Central Alarm Panel",
  "securityDefinitions": { "nosec_sc": { "scheme": "nosec" } },
  "security": "nosec_sc",
  "properties": {
    "status": {
      "type": "string",
      "forms": [{"href": "mqtt://149.132.176.75/christianferrario/museum/central_alarm/status"}]
    }
  }
}
)rawliteral";

// Funzione helper per trovare o creare un nodo nel registro
int getNodeIndex(String id) {
  for (int i = 0; i < activeNodeCount; i++) {
    if (nodeRegistry[i].id == id) {
      nodeRegistry[i].lastSeen = millis();
      return i;
    }
  }
  if (activeNodeCount < MAX_NODES) {
    nodeRegistry[activeNodeCount].id = id;
    nodeRegistry[activeNodeCount].state = ARMED;
    nodeRegistry[activeNodeCount].activeWarning = false;
    nodeRegistry[activeNodeCount].alarmReason = "";
    nodeRegistry[activeNodeCount].data.distanceCm = 999.0;
    nodeRegistry[activeNodeCount].lastSeen = millis();
    // Default thresholds for new nodes
    nodeRegistry[activeNodeCount].settings.tempMax = 30.0;
    nodeRegistry[activeNodeCount].settings.humMax = 60.0;
    nodeRegistry[activeNodeCount].settings.lightMax = 999;
    nodeRegistry[activeNodeCount].settings.distMin = 10;
    
    // Inizializza tutte le capabilities a false per sicurezza
    nodeRegistry[activeNodeCount].capabilities = {false, false, false, false, false, false, false, false};
    
    activeNodeCount++;
    return activeNodeCount - 1;
  }
  return -1; // Registro pieno
}

// ==========================================
// MQTT CALLBACK
// ==========================================
void onMqttMessage(String &topic, String &payload) {
  // WoT Discovery Topic per parsing dinamico
  if (topic == WOT_DISCOVERY_TOPIC) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      String urnId = doc["id"].as<String>();
      if (urnId.startsWith("urn:dev:mac:")) {
        String nodeId = urnId.substring(12);
        if (nodeId == "central_alarm") return; // Ignora la propria Thing Description
        
        int idx = getNodeIndex(nodeId);
        if (idx != -1) {
          nodeRegistry[idx].capabilities.hasTemperature = doc["properties"].containsKey("temperature");
          nodeRegistry[idx].capabilities.hasHumidity = doc["properties"].containsKey("humidity");
          nodeRegistry[idx].capabilities.hasLight = doc["properties"].containsKey("lightLevel");
          nodeRegistry[idx].capabilities.hasDistance = doc["properties"].containsKey("distanceCm");
          nodeRegistry[idx].capabilities.hasFlame = doc["properties"].containsKey("flameAnalog");
          
          nodeRegistry[idx].capabilities.hasImpactEvent = doc["events"].containsKey("impactDetected");
          nodeRegistry[idx].capabilities.hasFireEvent = doc["events"].containsKey("fireDetected");
          nodeRegistry[idx].capabilities.hasWarningEvent = doc["events"].containsKey("visitorWarning");
          
          Serial.println(">>> WoT TD Parsed for " + nodeId + " <<<");
        }
      }
    } else {
      Serial.println("Error parsing TD: " + String(error.c_str()));
    }
    return; // Non processare come normale topic di telemetria
  }

  // topic example: christianferrario/museum/teca_1A2B3C/telemetry
  int baseLen = String(MQTT_BASE_TOPIC).length();
  int nextSlash = topic.indexOf('/', baseLen);
  if (nextSlash == -1) return;

  String nodeId = topic.substring(baseLen, nextSlash);
  if (nodeId == "central_alarm") return; // Ignora i messaggi provenienti da se stesso

  String subTopic = topic.substring(nextSlash + 1);

  int idx = getNodeIndex(nodeId);
  if (idx == -1) return; 

  if (subTopic == "status") {
    if (payload == "offline") {
      Serial.println("!!! NODO DISCONNESSO: " + nodeId + " !!!");
      
      // Rimuovi il nodo traslando gli elementi successivi verso sinistra
      for (int i = idx; i < activeNodeCount - 1; i++) {
        nodeRegistry[i] = nodeRegistry[i + 1];
      }
      activeNodeCount--;
      
      // Correggi l'indice del display se puntava al nodo eliminato (o oltre)
      if (selectedNodeIndex >= activeNodeCount) {
        selectedNodeIndex = activeNodeCount > 0 ? activeNodeCount - 1 : 0;
      }
      return; // Nodo rimosso, non c'è altro da fare
    }
  } else if (subTopic == "events/impact") {
    if (payload == "true") {
      nodeRegistry[idx].state = ALARM_ACTIVE;
      nodeRegistry[idx].alarmReason = "IMPACT";
      Serial.println("!!! ALLARME INTRUSIONE DA " + nodeId + " !!!");
    }
  } else if (subTopic == "events/fire") {
    if (payload == "true") {
      nodeRegistry[idx].state = ALARM_ACTIVE;
      nodeRegistry[idx].alarmReason = "FIRE";
      Serial.println("!!! ALLARME INCENDIO DA " + nodeId + " !!!");
    }
  } else if (subTopic == "events/warning") {
    if (payload == "true") {
      nodeRegistry[idx].activeWarning = true;
      nodeRegistry[idx].data.distanceCm = 1.0; // Valore fittizio per forzare warning visivo
    } else {
      nodeRegistry[idx].activeWarning = false;
      nodeRegistry[idx].data.distanceCm = 999.0;
    }
  } else if (subTopic == "telemetry") {
    int tIdx = payload.indexOf("\"temperature\":");
    if (tIdx > 0) nodeRegistry[idx].data.temperature = payload.substring(tIdx + 14, payload.indexOf(",", tIdx)).toFloat();
    
    int hIdx = payload.indexOf("\"humidity\":");
    if (hIdx > 0) nodeRegistry[idx].data.humidity = payload.substring(hIdx + 11, payload.indexOf(",", hIdx)).toFloat();

    int lIdx = payload.indexOf("\"lightLevel\":");
    if (lIdx > 0) nodeRegistry[idx].data.lightLevel = payload.substring(lIdx + 13, payload.indexOf(",", lIdx)).toInt();

    int dIdx = payload.indexOf("\"distanceCm\":");
    if (dIdx > 0) nodeRegistry[idx].data.distanceCm = payload.substring(dIdx + 13, payload.indexOf(",", dIdx)).toFloat();

    int fIdx = payload.indexOf("\"flameAnalog\":");
    if (fIdx > 0) nodeRegistry[idx].data.flameAnalog = payload.substring(fIdx + 14, payload.indexOf(",", fIdx)).toInt();

    int sIdx = payload.indexOf("\"currentState\":");
    if (sIdx > 0) nodeRegistry[idx].state = (SystemState)payload.substring(sIdx + 15, payload.indexOf("}", sIdx)).toInt();
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
  
  showSetupMessage("CentralAlarm-Setup");
  networkManager.connect("CentralAlarm-Setup");

  setupWeb(); // Inizializza il Web Server Master

  mqttManager.setWill(MQTT_STATUS_TOPIC, "offline", true, 1);
  mqttManager.setup();
  mqttManager.onMessage(onMqttMessage);

  if (mqttManager.isConnected()) {
    mqttManager.publish(MQTT_STATUS_TOPIC, "online", true, 1);
    mqttManager.publish(WOT_DISCOVERY_TOPIC, thingDescription, true, 1);
    
    mqttManager.subscribe(WOT_DISCOVERY_TOPIC); // Sottoscriviti per le TDs degli altri
    mqttManager.subscribe(MQTT_WILDCARD_TELEMETRY);
    mqttManager.subscribe(MQTT_WILDCARD_IMPACT, 1); // QoS 1 per allarmi
    mqttManager.subscribe(MQTT_WILDCARD_FIRE, 1);   // QoS 1 per allarmi
    mqttManager.subscribe(MQTT_WILDCARD_WARNING, 1);
    mqttManager.subscribe(MQTT_WILDCARD_STATUS, 1); // QoS 1 per LWT
    Serial.println("Iscritto ai topic wildcard.");
  }

  Serial.println("==== CENTRAL ALARM BOOT ====");
  Serial.println("Master NodeMCU inizializzato con successo.");
  currentState = ARMED;
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  taskDisplay();
  handleWebTask();
  mqttManager.loop();

  // Re-subscribe if needed
  static bool wasConnected = true;
  if (mqttManager.isConnected() && !wasConnected) {
      mqttManager.subscribe(WOT_DISCOVERY_TOPIC);
      mqttManager.subscribe(MQTT_WILDCARD_TELEMETRY);
      mqttManager.subscribe(MQTT_WILDCARD_IMPACT, 1);
      mqttManager.subscribe(MQTT_WILDCARD_FIRE, 1);
      mqttManager.subscribe(MQTT_WILDCARD_WARNING, 1);
      mqttManager.subscribe(MQTT_WILDCARD_STATUS, 1);
      wasConnected = true;
  } else if (!mqttManager.isConnected()) {
      wasConnected = false;
  }

  // --- LOGICA ATTUATORI GLOBALI ---
  bool globalAlarm = false;
  bool globalWarning = false;

  for (int i = 0; i < activeNodeCount; i++) {
    if (nodeRegistry[i].state == ALARM_ACTIVE) {
      globalAlarm = true;
    }
    if (nodeRegistry[i].activeWarning || 
        nodeRegistry[i].data.temperature > nodeRegistry[i].settings.tempMax || 
        nodeRegistry[i].data.humidity > nodeRegistry[i].settings.humMax || 
        nodeRegistry[i].data.lightLevel > nodeRegistry[i].settings.lightMax) {
      globalWarning = true;
    }
  }

  if (globalAlarm) {
    currentState = ALARM_ACTIVE;
    playAlarm();
  } else if (globalWarning) {
    currentState = ARMED;
    playWarning();
  } else {
    currentState = ARMED;
    stopActuators();
  }

  // Controllo bottone encoder (Mute globale locale o navigazione UI)
  if (flagEncoderPressed) {
    flagEncoderPressed = false;
    // Se c'è un allarme, mutiamo tutti i nodi
    if (currentState == ALARM_ACTIVE) {
      for (int i = 0; i < activeNodeCount; i++) {
        if (nodeRegistry[i].state == ALARM_ACTIVE) {
          String topic = String(MQTT_BASE_TOPIC) + nodeRegistry[i].id + "/actions/cmd";
          mqttManager.publish(topic.c_str(), "MUTE");
          // Reset immediato dello stato locale per evitare loop dell'allarme in attesa della telemetria
          nodeRegistry[i].state = ARMED;
          nodeRegistry[i].alarmReason = "";
        }
      }
      currentState = ARMED;
      stopActuators();
      Serial.println("ALARM_ACK: Allarmi silenziati da pulsante locale.");
    } else {
      // Navigazione menu
      if (currentPage == STATE_OVERVIEW) {
        if (activeNodeCount > 0) currentPage = STATE_NODE_LIST;
      } else if (currentPage == STATE_NODE_LIST) {
        currentPage = STATE_NODE_DETAIL_1;
      } else if (currentPage == STATE_NODE_DETAIL_1 || currentPage == STATE_NODE_DETAIL_2) {
        currentPage = STATE_OVERVIEW;
      }
      extern unsigned long lastDisplayUpdate;
      lastDisplayUpdate = 0; // forza redraw
    }
  }
}
