#include "AlarmLogic.h"
#include "Config.h"
#include "MqttManager.h"
#include "TelegramManager.h"
#include "InfluxManager.h"

static MqttManager* _mqttManager = nullptr;
static TelegramManager* _telegramManager = nullptr;
static InfluxManager* _influxManager = nullptr;

void initAlarmLogic(MqttManager* mqtt, TelegramManager* tele, InfluxManager* influx) {
    _mqttManager = mqtt;
    _telegramManager = tele;
    _influxManager = influx;
}

void checkNodeThresholds(int i, bool &globalWarning) {
    unsigned long currentMillis = millis();

    // --- TEMPERATURA ---
    if (nodeRegistry[i].capabilities.hasTemperature && nodeRegistry[i].data.temperature > nodeRegistry[i].settings.tempMax) {
        globalWarning = true;
        if (nodeRegistry[i].tempTracker.exceededSince == 0) {
            nodeRegistry[i].tempTracker.exceededSince = currentMillis;
        } else if (!nodeRegistry[i].tempTracker.logged && (currentMillis - nodeRegistry[i].tempTracker.exceededSince > 10000)) {
            if (_influxManager) _influxManager->logThreshold(nodeRegistry[i].id, "temperature", nodeRegistry[i].data.temperature, nodeRegistry[i].settings.tempMax);
            nodeRegistry[i].tempTracker.logged = true;
        }
    } else {
        nodeRegistry[i].tempTracker.exceededSince = 0;
        nodeRegistry[i].tempTracker.logged = false;
    }

    // --- UMIDITÀ ---
    if (nodeRegistry[i].capabilities.hasHumidity && nodeRegistry[i].data.humidity > nodeRegistry[i].settings.humMax) {
        globalWarning = true;
        if (nodeRegistry[i].humTracker.exceededSince == 0) {
            nodeRegistry[i].humTracker.exceededSince = currentMillis;
        } else if (!nodeRegistry[i].humTracker.logged && (currentMillis - nodeRegistry[i].humTracker.exceededSince > 10000)) {
            if (_influxManager) _influxManager->logThreshold(nodeRegistry[i].id, "humidity", nodeRegistry[i].data.humidity, nodeRegistry[i].settings.humMax);
            nodeRegistry[i].humTracker.logged = true;
        }
    } else {
        nodeRegistry[i].humTracker.exceededSince = 0;
        nodeRegistry[i].humTracker.logged = false;
    }

    // --- LUCE ---
    if (nodeRegistry[i].capabilities.hasLight && nodeRegistry[i].data.lightLevel > nodeRegistry[i].settings.lightMax) {
        globalWarning = true;
        if (nodeRegistry[i].lightTracker.exceededSince == 0) {
            nodeRegistry[i].lightTracker.exceededSince = currentMillis;
        } else if (!nodeRegistry[i].lightTracker.logged && (currentMillis - nodeRegistry[i].lightTracker.exceededSince > 10000)) {
            if (_influxManager) _influxManager->logThreshold(nodeRegistry[i].id, "light", nodeRegistry[i].data.lightLevel, nodeRegistry[i].settings.lightMax);
            nodeRegistry[i].lightTracker.logged = true;
        }
    } else {
        nodeRegistry[i].lightTracker.exceededSince = 0;
        nodeRegistry[i].lightTracker.logged = false;
    }

    // Altri warning non legati ai 10 secondi
    if (nodeRegistry[i].activeWarning) {
        globalWarning = true;
        
        // --- PROSSIMITA' (10 Secondi per InfluxDB) ---
        if (nodeRegistry[i].distTracker.exceededSince == 0) {
            nodeRegistry[i].distTracker.exceededSince = currentMillis;
        } else if (!nodeRegistry[i].distTracker.logged && (currentMillis - nodeRegistry[i].distTracker.exceededSince > 10000)) {
            extern int thresh_distance_min;
            if (_influxManager) _influxManager->logThreshold(nodeRegistry[i].id, "proximity", nodeRegistry[i].data.distanceCm, thresh_distance_min);
            nodeRegistry[i].distTracker.logged = true;
        }
    } else {
        nodeRegistry[i].distTracker.exceededSince = 0;
        nodeRegistry[i].distTracker.logged = false;
    }
}

void muteAllAlarms() {
  for (int i = 0; i < activeNodeCount; i++) {
    if (nodeRegistry[i].state == ALARM_ACTIVE) {
      String topic = String(MQTT_BASE_TOPIC) + nodeRegistry[i].id + "/actions/cmd";
      if (_mqttManager) _mqttManager->publish(topic.c_str(), "MUTE");
      
      // Reset immediato dello stato locale per evitare loop dell'allarme in attesa della telemetria
      nodeRegistry[i].state = ARMED;
      nodeRegistry[i].alarmReason = "";
      
      // Se è il nodo virtuale di test manuale, rimuoviamolo per non inquinare la UI
      if (nodeRegistry[i].id == "manual_test") {
        for (int j = i; j < activeNodeCount - 1; j++) {
          nodeRegistry[j] = nodeRegistry[j + 1];
        }
        activeNodeCount--;
        i--; // Compensa per l'elemento rimosso
        
        if (selectedNodeIndex >= activeNodeCount) {
          selectedNodeIndex = activeNodeCount > 0 ? activeNodeCount - 1 : 0;
        }
      }
    }
  }
}

void triggerManualFire() {
  String topic = String(MQTT_BASE_TOPIC) + "manual_test/events/fire";
  if (_mqttManager) _mqttManager->publish(topic.c_str(), "true");
}

void triggerManualImpact() {
  String topic = String(MQTT_BASE_TOPIC) + "manual_test/events/impact";
  if (_mqttManager) _mqttManager->publish(topic.c_str(), "true");
}

void armAllNodes() {
  currentState = ARMED;
  for (int i = 0; i < activeNodeCount; i++) {
    String topic = String(MQTT_BASE_TOPIC) + nodeRegistry[i].id + "/actions/cmd";
    if (_mqttManager) _mqttManager->publish(topic.c_str(), "ARM");
  }
}

void disarmAllNodes() {
  currentState = DISARMED;
  for (int i = 0; i < activeNodeCount; i++) {
    String topic = String(MQTT_BASE_TOPIC) + nodeRegistry[i].id + "/actions/cmd";
    if (_mqttManager) _mqttManager->publish(topic.c_str(), "DISARM");
  }
}

String getSystemStatus() {
  String msg = "Stato Sistema:\n";
  msg += "Modalità: " + String(currentState == ARMED ? "ARMED 🛡️" : (currentState == DISARMED ? "DISARMED 🔓" : "ALARM 🚨")) + "\n";
  msg += "Nodi Connessi: " + String(activeNodeCount) + "\n\n";
  for (int i = 0; i < activeNodeCount; i++) {
    msg += "- " + nodeRegistry[i].id + " (" + (nodeRegistry[i].state == ALARM_ACTIVE ? "🚨" : "✅") + ")\n";
  }
  return msg;
}
