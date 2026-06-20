#include "Sensors.h"
#include "Config.h"
#include "State.h"
#include <DHT.h>
#include <ESP8266WiFi.h>

extern void addLog(String msg);

DHT dht(PIN_DHT, DHT_TYPE);

static unsigned long lastDhtRead = 0;
const unsigned long dhtInterval =
    3000; // DHT11 necessita almeno 2 secondi tra letture

static unsigned long lastSonarTrig = 0;
const unsigned long sonarInterval = 60;

// Variabili per l'interrupt asincrono dell'HC-SR04
volatile unsigned long echoStart = 0;
volatile unsigned long echoEnd = 0;
volatile bool echoReceived = false;
volatile bool isMeasuring = false;

// ==========================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ==========================================

void ICACHE_RAM_ATTR handleKnockInterrupt() { flagKnockDetected = true; }

void ICACHE_RAM_ATTR handleEchoInterrupt() {
  if (digitalRead(PIN_HC_ECHO) == HIGH) {
    // Il segnale sale: se non stavamo già misurando, registriamo l'inizio
    if (!isMeasuring) {
      echoStart = micros();
      isMeasuring = true;
    }
  } else {
    // Il segnale scende: onda tornata
    if (isMeasuring) {
      echoEnd = micros();
      isMeasuring = false;
      // Filtro antirimbalzo/rumore: ignoriamo impulsi microscopici (< 116us, ovvero < 2cm)
      if (echoEnd - echoStart > 116) {
        echoReceived = true;
      }
    }
  }
}

// ==========================================
// SETUP
// ==========================================
void setupSensors() {
  // LDR Light Sensor Setup
  pinMode(PIN_LDR, INPUT);

  // In caso di riavvio software l'integrato si blocca (latch-up) aspettando un
  // fine transazione che non arriverà. Lo forziamo HIGH per 250ms per
  // resettargli la macchina a stati.
  pinMode(PIN_DHT, OUTPUT);
  digitalWrite(PIN_DHT, HIGH);
  delay(250);

  dht.begin();

  // Knock / Hit Sensor Setup
  pinMode(PIN_KNOCK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KNOCK), handleKnockInterrupt,
                  FALLING);

  // HC-SR04 Ultrasonic Sensor Setup
  pinMode(PIN_HC_TRIG, OUTPUT);
  digitalWrite(PIN_HC_TRIG, LOW);
  pinMode(PIN_HC_ECHO, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_HC_ECHO), handleEchoInterrupt,
                  CHANGE);
}

// ==========================================
// EXECUTION TASK PER POLLING ASINCRONO
// ==========================================
void taskSensori() {
  unsigned long currentMillis = millis();

  static unsigned long lastLdrRead = 0;
  if (currentMillis - lastLdrRead >= 500) {
    lastLdrRead = currentMillis;
    currentData.lightLevel = analogRead(PIN_LDR);
  }

  // 2. LETTURA DHT11
  if (currentMillis - lastDhtRead >= dhtInterval) {
    lastDhtRead = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      currentData.humidity = h;
      currentData.temperature = t;
    } else {
      addLog("Errore sensore DHT11: NaN"); // Debug per mancata comunicazione
    }
  }

  // 3. LETTURA HC-SR04 CON GESTIONE ASINCRONA

  if (currentMillis - lastSonarTrig >= sonarInterval) {
    lastSonarTrig = currentMillis;
    isMeasuring = false;
    echoReceived = false;
    digitalWrite(PIN_HC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_HC_TRIG, LOW);
  }

  if (echoReceived) {
    echoReceived = false;
    long duration = echoEnd - echoStart;

    if (duration > 0) {
      float distance = (duration * 0.0343) / 2.0;
      
      // Il sensore HC-SR04 ha un range di circa 2cm - 400cm.
      // Tempi troppo brevi (sotto i 116us, ovvero < 2cm) spesso indicano 
      // un errore di lettura o che non c'è nessun ostacolo (timeout dell'onda).
      // Se fuori range, impostiamo una distanza altissima (es. 999) per non far scattare allarmi.
      if (distance < 2.0 || distance > 400.0) {
        currentData.distanceCm = 999.0;
      } else {
        currentData.distanceCm = distance;
      }
    }
  }
}
