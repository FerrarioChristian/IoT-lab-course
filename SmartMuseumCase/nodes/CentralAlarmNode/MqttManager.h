#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <ESP8266WiFi.h>
#include <MQTT.h>

class MqttManager {
public:
    MqttManager(const char* brokerIp, const char* clientId, const char* username, const char* password);
    
    // Configura Last Will prima di connettersi
    void setWill(const char* topic, const char* payload, bool retained = false, int qos = 0);
    
    // Da chiamare nel setup() di Arduino
    void setup();
    
    // Da chiamare nel loop() di Arduino per mantenere viva la connessione
    void loop();
    
    // Wrapper per pubblicare e iscriversi
    bool publish(const char* topic, const char* payload, bool retained = false, int qos = 0);
    bool subscribe(const char* topic, int qos = 0);
    void onMessage(MQTTClientCallbackSimple cb);
    
    bool isConnected();
    
private:
    void connect();
    const char* _brokerIp;
    const char* _clientId;
    const char* _username;
    const char* _password;
    WiFiClient _net;
    MQTTClient _client;
    unsigned long _lastConnectAttempt;
};

#endif // MQTT_MANAGER_H