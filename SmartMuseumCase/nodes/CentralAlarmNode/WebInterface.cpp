#include "WebInterface.h"
#include "Config.h"
#include "Dashboard.h"
#include "State.h"
#include "MqttManager.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

extern MqttManager mqttManager;
ESP8266WebServer server(80);

// ==========================================
// SIMULATORE SERIALE OVER-THE-AIR (OTA LOG)
// ==========================================
const int MAX_LOGS = 20;
String logBuffer[MAX_LOGS];
int logIndex = 0;

void addLog(String msg) {
  unsigned long ms = millis();
  float sec = ms / 1000.0;
  logBuffer[logIndex] = "[" + String(sec, 2) + "s] " + msg;
  logIndex = (logIndex + 1) % MAX_LOGS;
  Serial.println(msg);
}

void handleDebugPage() { server.send_P(200, "text/html", DEBUG_HTML); }

void handleApiDebugData() {
  String out = "";
  for (int i = 0; i < MAX_LOGS; i++) {
    int idx = (logIndex + i) % MAX_LOGS;
    if (logBuffer[idx].length() > 0) {
      out += logBuffer[idx] + "\n";
    }
  }
  server.send(200, "text/plain", out);
}

void handleSettingsRoute() { server.send_P(200, "text/html", SETTINGS_HTML); }

void handleApiSettingsGetRoute() {
  if (!server.hasArg("nodeId")) {
    server.send(400, "text/plain", "Missing nodeId");
    return;
  }
  String nodeId = server.arg("nodeId");
  
  int idx = -1;
  for (int i=0; i<activeNodeCount; i++) {
    if (nodeRegistry[i].id == nodeId) { idx = i; break; }
  }

  if (idx == -1) {
    server.send(404, "text/plain", "Node not found");
    return;
  }

  String json = "{";
  json += "\"dist\":" + String(nodeRegistry[idx].settings.distMin) + ",";
  json += "\"light\":" + String(nodeRegistry[idx].settings.lightMax) + ",";
  json += "\"hum\":" + String(nodeRegistry[idx].settings.humMax) + ",";
  json += "\"temp\":" + String(nodeRegistry[idx].settings.tempMax) + ",";
  json += "\"uv\":" + String(thresh_uv_max);
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiSettingsPostRoute() {
  if (!server.hasArg("nodeId")) {
    server.send(400, "text/plain", "Missing nodeId");
    return;
  }
  String nodeId = server.arg("nodeId");

  int idx = -1;
  for (int i=0; i<activeNodeCount; i++) {
    if (nodeRegistry[i].id == nodeId) { idx = i; break; }
  }

  if (idx == -1) {
    server.send(404, "text/plain", "Node not found");
    return;
  }

  if (server.hasArg("dist")) nodeRegistry[idx].settings.distMin = server.arg("dist").toInt();
  if (server.hasArg("light")) nodeRegistry[idx].settings.lightMax = server.arg("light").toInt();
  if (server.hasArg("hum")) nodeRegistry[idx].settings.humMax = server.arg("hum").toFloat();
  if (server.hasArg("temp")) nodeRegistry[idx].settings.tempMax = server.arg("temp").toFloat();
  if (server.hasArg("uv")) thresh_uv_max = server.arg("uv").toFloat();
  
  // Opzionale: Inviare il comando MQTT alla teca per aggiornare la soglia di prossimità hardware
  String topic = String(MQTT_BASE_TOPIC) + nodeId + "/settings/distance";
  mqttManager.publish(topic.c_str(), String(nodeRegistry[idx].settings.distMin).c_str(), true, 1);

  server.send(200, "text/plain", "OK");
}

// ==========================================
// ROUTE HANDLERS
// ==========================================

void handleRootRoute() { server.send_P(200, "text/html", DASHBOARD_HTML); }

void handleApiDataRoute() {
  String jsonString = "[";
  for (int i = 0; i < activeNodeCount; i++) {
    if (i > 0) jsonString += ",";
    jsonString += "{";
    jsonString += "\"id\":\"" + nodeRegistry[i].id + "\",";
    jsonString += "\"temp\":" + String(nodeRegistry[i].data.temperature) + ",";
    jsonString += "\"hum\":" + String(nodeRegistry[i].data.humidity) + ",";
    jsonString += "\"light\":" + String(nodeRegistry[i].data.lightLevel) + ",";
    jsonString += "\"dist\":" + String(nodeRegistry[i].data.distanceCm) + ",";
    jsonString += "\"flame\":" + String(nodeRegistry[i].data.flameAnalog) + ",";

    jsonString += "\"caps\":{";
    jsonString += "\"temp\":" + String(nodeRegistry[i].capabilities.hasTemperature ? "true" : "false") + ",";
    jsonString += "\"hum\":" + String(nodeRegistry[i].capabilities.hasHumidity ? "true" : "false") + ",";
    jsonString += "\"light\":" + String(nodeRegistry[i].capabilities.hasLight ? "true" : "false") + ",";
    jsonString += "\"dist\":" + String(nodeRegistry[i].capabilities.hasDistance ? "true" : "false") + ",";
    jsonString += "\"flame\":" + String(nodeRegistry[i].capabilities.hasFlame ? "true" : "false");
    jsonString += "},";

    String stateStr;
    if (nodeRegistry[i].state == ARMED)
      stateStr = "ARMED";
    else if (nodeRegistry[i].state == DISARMED)
      stateStr = "DISARMED";
    else
      stateStr = "ALARM_ACTIVE";

    jsonString += "\"state\":\"" + stateStr + "\"";
    jsonString += "}";
  }
  jsonString += "]";
  server.send(200, "application/json", jsonString);
}

void handleApiActionRoute() {
  if (server.hasArg("cmd") && server.hasArg("nodeId")) {
    String cmd = server.arg("cmd");
    String nodeId = server.arg("nodeId");
    
    String topic = String(MQTT_BASE_TOPIC) + nodeId + "/actions/cmd";
    mqttManager.publish(topic.c_str(), cmd.c_str());
    addLog("Comando " + cmd + " inviato a " + nodeId);
  }
  server.send(200, "text/plain", "OK");
}

// ==========================================
// INITIALIZATION
// ==========================================

void setupWeb() {
  server.on("/", HTTP_GET, handleRootRoute);
  server.on("/api/data", HTTP_GET, handleApiDataRoute);
  server.on("/api/action", HTTP_POST, handleApiActionRoute);
  server.on("/debug", HTTP_GET, handleDebugPage);
  server.on("/api/debug_data", HTTP_GET, handleApiDebugData);
  server.on("/settings", HTTP_GET, handleSettingsRoute);
  server.on("/api/settings", HTTP_GET, handleApiSettingsGetRoute);
  server.on("/api/settings", HTTP_POST, handleApiSettingsPostRoute);

  server.begin();
}

void handleWebTask() {
  server.handleClient();
}