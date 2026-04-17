#include "WebInterface.h"
#include "Config.h"
#include "Dashboard.h"
#include "Database.h"
#include "State.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

ESP8266WebServer server(80);

// ==========================================
// SIMULATORE SERIALE OVER-THE-AIR (OTA LOG)
// ==========================================
const int MAX_LOGS = 20; // Conserva le ultime 20 righe
String logBuffer[MAX_LOGS];
int logIndex = 0;

void addLog(String msg) {
  unsigned long ms = millis();
  float sec = ms / 1000.0;
  // Format: [12.45s] Messaggio
  logBuffer[logIndex] = "[" + String(sec, 2) + "s] " + msg;
  logIndex = (logIndex + 1) % MAX_LOGS;
}

void handleDebugPage() {
  server.send_P(200, "text/html", DEBUG_HTML);
}

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

void handleSettingsRoute() {
  server.send_P(200, "text/html", SETTINGS_HTML);
}

void handleApiSettingsGetRoute() {
  String json = "{";
  json += "\"dist\":" + String(thresh_distance_min) + ",";
  json += "\"light\":" + String(thresh_light_max) + ",";
  json += "\"hum\":" + String(thresh_hum_max) + ",";
  json += "\"temp\":" + String(thresh_temp_max);
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiSettingsPostRoute() {
  if (server.hasArg("dist")) thresh_distance_min = server.arg("dist").toInt();
  if (server.hasArg("light")) thresh_light_max = server.arg("light").toInt();
  if (server.hasArg("hum")) thresh_hum_max = server.arg("hum").toFloat();
  if (server.hasArg("temp")) thresh_temp_max = server.arg("temp").toFloat();
  server.send(200, "text/plain", "OK");
}

// ==========================================
// ROUTE HANDLERS
// ==========================================

void handleRootRoute() {
  // Restituisce la dashboard HTML salvata in PROGMEM, velocissimo ed
  // efficiente.
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleApiDataRoute() {
  // Assembliamo un rudimentale ma eficientist JSON nativo senza librerie
  // esterne giganti
  String jsonString = "{";
  jsonString += "\"temp\":" + String(currentData.temperature) + ",";
  jsonString += "\"hum\":" + String(currentData.humidity) + ",";
  jsonString += "\"light\":" + String(currentData.lightLevel) + ",";
  jsonString += "\"dist\":" + String(currentData.distanceCm) + ",";

  // Traduciamo lo stato in stringa Enum
  String stateStr;
  if (currentState == ARMED)
    stateStr = "ARMED";
  else if (currentState == DISARMED)
    stateStr = "DISARMED";
  else
    stateStr = "ALARM_ACTIVE";

  jsonString += "\"state\":\"" + stateStr + "\"";
  jsonString += "}";

  server.send(200, "application/json", jsonString);
}

void handleApiActionRoute() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "ARM") {
      if (currentState != ARMED)
        logSystemEvent("SYSTEM_ARMED", "Armed via Web API.");
      currentState = ARMED;
    } else if (cmd == "DISARM") {
      if (currentState != DISARMED)
        logSystemEvent("SYSTEM_DISARMED", "Disarmed via Web API.");
      currentState = DISARMED;
    } else if (cmd == "MUTE" && currentState == ALARM_ACTIVE) {
      currentState = ARMED;
      logSystemEvent("ALARM_ACK", "Silenced via Web API.");
    }
  }
  // Ritorna semplice ok
  server.send(200, "text/plain", "OK");
}

// ==========================================
// INITIALIZATION
// ==========================================

void setupWeb() {
  // Imposta modalità Station
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // NOTA: il while seguente è parzialmente bloccante (di proposito al boot)
  // Non avendo un display che scrolla testo fluido, è accettabile.
  // L'hardware resta fermo qui dentro durante i primi 3-4 secondi.
  // Se fallisce, prosegue, ma senza IP non c'è server.
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }

  // Mappa le Rotte URI sulle funzioni
  server.on("/", HTTP_GET, handleRootRoute);
  server.on("/api/data", HTTP_GET, handleApiDataRoute);
  server.on("/api/action", HTTP_POST, handleApiActionRoute);
  server.on("/debug", HTTP_GET, handleDebugPage);
  server.on("/api/debug_data", HTTP_GET, handleApiDebugData);
  server.on("/settings", HTTP_GET, handleSettingsRoute);
  server.on("/api/settings", HTTP_GET, handleApiSettingsGetRoute);
  server.on("/api/settings", HTTP_POST, handleApiSettingsPostRoute);

  // Avvia Server Letale
  server.begin();
}

void handleWebTask() {
  // Gestione asincrona della riconnessione WiFi senza bloccare mai il loop
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck >= 5000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      addLog("WiFi persa! Forzo riconnessione hardware...");
      WiFi.reconnect(); 
    }
  }

  // Rispondiamo tramite web server solo se l'hardware è attualmente connesso
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}
