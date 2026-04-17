#include "Sensors.h"
#include "Config.h"
#include "State.h"
#include "WebInterface.h"
#include <DHT.h>

DHT dht(PIN_DHT, DHT_TYPE);

static unsigned long lastDhtRead = 0;
const unsigned long dhtInterval = 3000; // DHT11 necessita 2 secondi tra letture

static unsigned long lastSonarTrig = 0;
const unsigned long sonarInterval = 60; // Frequenza ping sonar (60ms)

// Variabili per l'interrupt asincrono dell'HC-SR04
volatile unsigned long echoStart = 0;
volatile unsigned long echoEnd = 0;
volatile bool echoReceived = false;

// ==========================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ==========================================

// ISR Sensore d'impatto (deve essere definita nella RAM)
void ICACHE_RAM_ATTR handleKnockInterrupt() { flagKnockDetected = true; }

// ISR per leggere il tempo di volo del suono asincronamente
void ICACHE_RAM_ATTR handleEchoInterrupt() {
  if (digitalRead(PIN_HC_ECHO) == HIGH) {
    // Il segnale sale: parte l'onda sonora
    echoStart = micros();
  } else {
    // Il segnale scende: onda tornata
    echoEnd = micros();
    echoReceived = true;
  }
}

// ==========================================
// SETUP
// ==========================================
void setupSensors() {
  // LDR Light Sensor Setup
  pinMode(PIN_LDR, INPUT);

  // DHT11 Temperature and Humidity Sensor Setup
  dht.begin();

  // Knock / Hit Sensor Setup
  pinMode(PIN_KNOCK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KNOCK), handleKnockInterrupt,
                  FALLING);

  // HC-SR04 Ultrasonic Sensor Setup
  pinMode(PIN_HC_TRIG, OUTPUT);
  digitalWrite(PIN_HC_TRIG, LOW);
  pinMode(PIN_HC_ECHO, INPUT);

  // Interrupt in CHANGE perché ci serve calcolare l'intervallo tra il fronte di
  // salita e quello di discesa
  attachInterrupt(digitalPinToInterrupt(PIN_HC_ECHO), handleEchoInterrupt,
                  CHANGE);
}

// ==========================================
// EXECUTION TASK PER POLLING ASINCRONO
// ==========================================
void taskSensori() {
  unsigned long currentMillis = millis();

  // 1. LETTURA ANALOGICA LDR
  // L'ADC dell'ESP8266 è condiviso con il modulo Wi-Fi per la calibrazione di
  // potenza TX. Spammarlo a 100.000 hz nel loop causa il collasso delle
  // performance radio (Pagine web lente)!
  static unsigned long lastLdrRead = 0;
  if (currentMillis - lastLdrRead >= 500) {
    lastLdrRead = currentMillis;
    currentData.lightLevel = analogRead(PIN_LDR);
  }

  // 2. LETTURA DHT11
  if (currentMillis - lastDhtRead >= dhtInterval) {
    lastDhtRead = currentMillis;
    // Le librerie moderne DHT hanno letture veloci senza blocchi prolungati
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
  // Invia un impulso (Trigger) a intervalli regolari (non usiamo
  // delayMicroseconds lunghi!)
  if (currentMillis - lastSonarTrig >= sonarInterval) {
    lastSonarTrig = currentMillis;
    digitalWrite(PIN_HC_TRIG, HIGH);
    // Ritardo di soli 10 microsecondi è ininfluente per il server NodeMCU
    delayMicroseconds(10);
    digitalWrite(PIN_HC_TRIG, LOW);
  }

  // Controllo l'arrivo dei dati dall'Interrupt
  if (echoReceived) {
    echoReceived = false;
    long duration = echoEnd - echoStart;

    // Filtriamo letture scartinate dal rumore elettrico (23000ms corrispondono
    // a ca. 4 metri, limite sensoristico)
    if (duration > 0 && duration < 23000) {
      float distance = (duration * 0.0343) / 2.0;
      currentData.distanceCm = distance;

      // Check threshold di pre-allarme locale se siamo Armati
      if (currentState == ARMED && distance < thresh_distance_min) {
        // Pre-Allarme! Qui possiamo attivare lampeggio LED verde o altro.
        // Verrà gestito specificatamente nel modulo UI.
      }
    }
  }
}
