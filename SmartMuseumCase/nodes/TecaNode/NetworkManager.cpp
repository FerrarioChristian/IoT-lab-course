#include "NetworkManager.h"

NetworkManager::NetworkManager() {}

void NetworkManager::connect(const char *apName) {
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it
  // around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(apName);

  // if you get here you have connected to the WiFi
  Serial.println("Connected to WiFi.");
}