#ifndef INFLUX_MANAGER_H
#define INFLUX_MANAGER_H

#include <Arduino.h>
#include <InfluxDbClient.h>

class InfluxManager {
private:
    InfluxDBClient* client;
    bool isConfigured;

public:
    InfluxManager();
    ~InfluxManager();

    void setup(const char* url, const char* org, const char* bucket, const char* token);
    
    // Logga un evento immediato (es. FIRE o IMPACT)
    void logEvent(String nodeId, String eventType);

    // Logga un'anomalia di una soglia assieme al valore (dopo 10s)
    void logThreshold(String nodeId, String sensorType, float value, float threshold);
};

#endif // INFLUX_MANAGER_H
