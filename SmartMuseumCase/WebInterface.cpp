#include "WebInterface.h"
#include "Config.h"
#include "State.h"
#include "Dashboard.h"
#include "Database.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// ==========================================
// ROUTE HANDLERS
// ==========================================

void handleRootRoute() {
    // Restituisce la dashboard HTML salvata in PROGMEM, velocissimo ed efficiente.
    server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleApiDataRoute() {
    // Assembliamo un rudimentale ma eficientist JSON nativo senza librerie esterne giganti
    String jsonString = "{";
    jsonString += "\"temp\":" + String(currentData.temperature) + ",";
    jsonString += "\"hum\":" + String(currentData.humidity) + ",";
    jsonString += "\"light\":" + String(currentData.lightLevel) + ",";
    jsonString += "\"dist\":" + String(currentData.distanceCm) + ",";
    
    // Traduciamo lo stato in stringa Enum
    String stateStr;
    if (currentState == ARMED) stateStr = "ARMED";
    else if (currentState == DISARMED) stateStr = "DISARMED";
    else stateStr = "ALARM_ACTIVE";
    
    jsonString += "\"state\":\"" + stateStr + "\"";
    jsonString += "}";
    
    server.send(200, "application/json", jsonString);
}

void handleApiActionRoute() {
    if (server.hasArg("cmd")) {
        String cmd = server.arg("cmd");
        if (cmd == "ARM") {
            if (currentState != ARMED) logSystemEvent("SYSTEM_ARMED", "Armed via Web API.");
            currentState = ARMED;
        } 
        else if (cmd == "DISARM") {
            if (currentState != DISARMED) logSystemEvent("SYSTEM_DISARMED", "Disarmed via Web API.");
            currentState = DISARMED;
        } 
        else if (cmd == "MUTE" && currentState == ALARM_ACTIVE) {
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
    
    // Avvia Server Letale
    server.begin();
}

void handleWebTask() {
    server.handleClient();
}
