#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

class NetworkManager {
public:
    NetworkManager();
    void connect(const char* apName);
};

#endif // NETWORK_MANAGER_H