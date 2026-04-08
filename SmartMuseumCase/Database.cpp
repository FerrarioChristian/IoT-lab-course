#include "Database.h"
#include "Config.h"
#include "State.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Definizione del client InfluxDB (Costruttore base senza Cloud Cert rigidi per
// locale IoT) SetInsecure permette la connessione a server locali Influx senza
// scontrarsi con SSL stretti o se Influx e HTTP puro
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Definizione delle misurazioni (punti logici su Influx)
Point telemetry("telemetry");
Point systemEvents("system_events");

unsigned long lastTelemetrySync = 0;
const unsigned long telemetryInterval =
    10000; // Scrive su DB ogni 10 secondi (bilanciamento perfetto rete/peso)

// ==========================================
// STARTUP
// ==========================================
void setupDatabase() {
  // Sincronizzazione Oraria via NTP
  // I TimeSeries DB (InfluxDB) impazziscono se l'hardware non sa che ore sono
  // (metterebbe anno 1970). Questo è un passaggio fondamentale per i log a
  // prova di manomissione.
  timeSync("UTC", "pool.ntp.org", "time.nis.gov");

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Scrive subito l'evento di un riavvio effettivo
  logSystemEvent("SYSTEM_BOOT", "Hardware ESP8266 riavviato e online.");
}

// ==========================================
// TELEMETRIA PERIODICA TRAMITE MILLIS
// ==========================================
void taskDatabase() {
  unsigned long currentMillis = millis();

  // Ogni 10 secondi preleviamo i dati attuali RAM e li sputiamo in C++
  if (currentMillis - lastTelemetrySync >= telemetryInterval) {
    lastTelemetrySync = currentMillis;

    // Reset buffer
    telemetry.clearFields();

    // Assegnazione Label Generica (per permettere query singole "Where Teca =
    // A")
    telemetry.clearTags();
    telemetry.addTag("device_id", "DisplayCase_01");
    telemetry.addTag("location", "Room_B");

    // Sensori
    telemetry.addField("temperature", currentData.temperature);
    telemetry.addField("humidity", currentData.humidity);
    telemetry.addField("light_level", currentData.lightLevel);
    telemetry.addField("proximity_cm", currentData.distanceCm);

    // Conversione Stato Sistema in intero per grafici (0=Disarm, 1=Arm,
    // 2=Alarm)
    int stateCode = 0;
    if (currentState == ARMED)
      stateCode = 1;
    if (currentState == ALARM_ACTIVE)
      stateCode = 2;
    telemetry.addField("internal_state", stateCode);

    // Non bloccante per l'intera durata se la connessione rete non fa capricci
    client.writePoint(telemetry);
  }
}

// ==========================================
// TRIGGER LOG DI STATO DISCRETO
// ==========================================
void logSystemEvent(const char* eventName, const char* details) {
  systemEvents.clearFields();
  systemEvents.clearTags();

  systemEvents.addTag("device_id", "DisplayCase_01");
  systemEvents.addField("type", eventName);

  if (details && details[0] != '\0') {
    systemEvents.addField("note", details);
  }

  client.writePoint(systemEvents);
}
