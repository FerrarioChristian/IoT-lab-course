#include "InfluxManager.h"

InfluxManager::InfluxManager() {
    client = nullptr;
    isConfigured = false;
}

InfluxManager::~InfluxManager() {
    if (client) delete client;
}

void InfluxManager::setup(const char* url, const char* org, const char* bucket, const char* token) {
    if (String(url) == "IL_TUO_URL_QUI" || String(url) == "") {
        Serial.println("InfluxManager: InfluxDB non configurato in Config.h (URL assente).");
        isConfigured = false;
        return;
    }

    client = new InfluxDBClient(url, org, bucket, token);
    
    // Configura per ignorare certificati SSL se richiesto (spesso utile su ESP8266 se non si usano certificati completi)
    client->setInsecure();

    if (client->validateConnection()) {
        Serial.print("InfluxManager: Connesso a InfluxDB: ");
        Serial.println(client->getServerUrl());
        isConfigured = true;
    } else {
        Serial.print("InfluxManager: Connessione a InfluxDB FALLITA: ");
        Serial.println(client->getLastErrorMessage());
        isConfigured = false;
    }
}

void InfluxManager::logEvent(String nodeId, String eventType) {
    if (!isConfigured || !client) return;

    Point point("museum_events");
    point.addTag("node_id", nodeId);
    point.addField("event", eventType);

    Serial.print("InfluxManager: Scrivo evento -> ");
    Serial.println(point.toLineProtocol());

    if (!client->writePoint(point)) {
        Serial.print("InfluxManager Errore scrittura evento: ");
        Serial.println(client->getLastErrorMessage());
    }
}

void InfluxManager::logThreshold(String nodeId, String sensorType, float value, float threshold) {
    if (!isConfigured || !client) return;

    Point point("museum_thresholds");
    point.addTag("node_id", nodeId);
    point.addTag("sensor", sensorType);
    point.addField("value", value);
    point.addField("threshold", threshold);

    Serial.print("InfluxManager: Scrivo anomalia soglia (>10s) -> ");
    Serial.println(point.toLineProtocol());

    if (!client->writePoint(point)) {
        Serial.print("InfluxManager Errore scrittura soglia: ");
        Serial.println(client->getLastErrorMessage());
    }
}
