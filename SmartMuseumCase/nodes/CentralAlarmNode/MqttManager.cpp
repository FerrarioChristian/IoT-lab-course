#include "MqttManager.h"

// Inizializza il client MQTT con un buffer di 1024 byte per supportare i file JSON grandi (Thing Description)
MqttManager::MqttManager(const char* brokerIp, const char* clientId, const char* username, const char* password) 
  : _client(1024) {
    _brokerIp = brokerIp;
    _clientId = clientId;
    _username = username;
    _password = password;
    _lastConnectAttempt = 0;
}

void MqttManager::setWill(const char* topic, const char* payload, bool retained, int qos) {
    _client.setWill(topic, payload, retained, qos);
}

void MqttManager::setup() {
    _client.begin(_brokerIp, 1883, _net);
    connect();
}

void MqttManager::connect() {
    Serial.print("MQTT connecting to ");
    Serial.print(_brokerIp);
    Serial.print(" as ");
    Serial.print(_clientId);
    Serial.print("...");
    
    if (_client.connect(_clientId, _username, _password)) {
        Serial.println(" connected!");
    } else {
        Serial.println(" failed!");
    }
}

void MqttManager::loop() {
    _client.loop();
    delay(10); // Piccolo delay per la stabilità del WiFi dell'ESP8266
    
    if (!_client.connected()) {
        unsigned long now = millis();
        // Riprova a connettersi ogni 5 secondi
        if (now - _lastConnectAttempt > 5000) {
            _lastConnectAttempt = now;
            connect();
        }
    }
}

bool MqttManager::publish(const char* topic, const char* payload, bool retained, int qos) {
    if (_client.connected()) {
        return _client.publish(topic, payload, retained, qos);
    }
    return false;
}

bool MqttManager::subscribe(const char* topic, int qos) {
    if (_client.connected()) {
        return _client.subscribe(topic, qos);
    }
    return false;
}

void MqttManager::onMessage(MQTTClientCallbackSimple cb) {
    _client.onMessage(cb);
}

bool MqttManager::isConnected() {
    return _client.connected();
}