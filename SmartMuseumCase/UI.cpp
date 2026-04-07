#include "UI.h"
#include "Config.h"
#include "State.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Definiamo il classico schermo LCD 16x2 all'indirizzo hardware base 0x27 (a
// volte può essere 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variabili asincrone per decodifica Encoder
volatile int lastEncoded = 0;

// Tempistiche LCD
unsigned long lastUiUpdate = 0;
const unsigned long uiInterval = 300; // Aggiorna LCD 3 volte al secondo

// Variabili per il lampeggio non bloccante Green LED
unsigned long lastLedBlink = 0;
bool ledGreenState = false;

// ==========================================
// ISR ROTARY ENCODER
// ==========================================
void ICACHE_RAM_ATTR handleEncoderInterrupt() {
  int MSB = digitalRead(PIN_ENC_CLK); // Most significant bit
  int LSB = digitalRead(PIN_ENC_DT);  // Least significant bit

  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  // A seconda della combinazione binaria passata/presente calcoliamo il verso
  // di rotazione
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderCount++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderCount--;

  lastEncoded = encoded;
}

// ISR per il pulsante del Rotary Encoder
void ICACHE_RAM_ATTR handleEncoderButton() { flagEncoderPressed = true; }

// ==========================================
// FUNZIONI HELPERS LCD
// ==========================================
void drawPageEnv() {
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(currentData.temperature, 1);
  lcd.print("C H:");
  lcd.print(currentData.humidity, 0);
  lcd.print("%  "); // Spazi per overwrite sporco precedente

  lcd.setCursor(0, 1);
  lcd.print("Luce:");
  lcd.print(currentData.lightLevel);
  lcd.print("         "); // Cancella ghost char
}

void drawPageDistance() {
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(currentData.distanceCm, 1);
  lcd.print(" cm     ");

  lcd.setCursor(0, 1);
  if (currentState == ARMED) {
    lcd.print("Stato: ARMATO   ");
  } else if (currentState == DISARMED) {
    lcd.print("Stato: DISARMATO");
  } else {
    lcd.print("!! ALLARME !!   ");
  }
}

void drawPageWiFi() {
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print("Disconn "); // Placeholder
  lcd.setCursor(0, 1);
  lcd.print("IP: Nessuno     ");
}

void updateDisplay() {
  switch (currentPage) {
  case PAGE_ENV:
    drawPageEnv();
    break;
  case PAGE_DISTANCE:
    drawPageDistance();
    break;
  case PAGE_WIFI:
    drawPageWiFi();
    break;
  }
}

// Navigazione pagine
void changePage(int direction) {
  int next = (int)currentPage + direction;
  if (next > 2)
    next = 0; // Wrap around
  if (next < 0)
    next = 2; // Wrap around
  currentPage = (PageState)next;
  lcd.clear();      // Pulisce lo schermo solo al cambio pagina per evitare
                    // sfarfallii
  lastUiUpdate = 0; // Forza aggiornamento immediato della vista
}

// ==========================================
// SETUP & EXECUTIONS
// ==========================================
void setupUI() {
  // I2C
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inizializza lo schermo e accende la backlight
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Smart Museum ");
  lcd.setCursor(0, 1);
  lcd.print("   Booting... ");

  // Setup PIN LED uscenti
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);

  // Setup ingressi Encoder con attivazione delle pull-up interne per sicurezza
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);

  // 3 Hardware interrupts. Uno a CHANGE per leggere la rotella, e FALLING per
  // bottone (premuto verso ground)
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoderInterrupt,
                  CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), handleEncoderInterrupt,
                  CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), handleEncoderButton,
                  FALLING);
}

void taskUI() {
  unsigned long currentMillis = millis();

  // 1. Controllo rotazione Encoder per cambio pagina asincrono
  static int localLastCount = 0;
  // Poiché ogni scatto meccanico in teoria genera due iterazioni gray-code o 4,
  // l'encoderCount potrebbe saltare di step (es. +4).
  if (encoderCount >= localLastCount + 4 ||
      encoderCount <= localLastCount - 4) {
    if (encoderCount > localLastCount)
      changePage(1);
    else
      changePage(-1);
    localLastCount = encoderCount;
  }

  // 2. Logic LED Indicator / Buzzer
  if (currentState == ALARM_ACTIVE) {
    digitalWrite(PIN_LED_RED, HIGH);  // LED Rosso/Sirena acceso (Stato Critico)
    digitalWrite(PIN_LED_GREEN, LOW); // Verde sempre spento
  } else if (currentState == ARMED &&
             currentData.distanceCm < thresh_distance_min) {
    // Warning PRE-ALLARME (Troppo vicini)
    digitalWrite(PIN_LED_RED, LOW);
    if (currentMillis - lastLedBlink >=
        150) { // Lampeggio rapido verde asincrono
      lastLedBlink = currentMillis;
      ledGreenState = !ledGreenState;
      digitalWrite(PIN_LED_GREEN, ledGreenState ? HIGH : LOW);
    }
  } else {
    // Nessun allarme: Verde Fisso se ARMATO, tutto spento se DISARMATO
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, (currentState == ARMED) ? HIGH : LOW);
  }

  // 3. Modifica temporizzata display e priorità degli allarmi a schermo
  if (currentMillis - lastUiUpdate >= uiInterval) {
    lastUiUpdate = currentMillis;

    if (currentState == ALARM_ACTIVE) {
      lcd.setCursor(0, 0);
      lcd.print("!! INTRUSIONE !!"); // Override totale dello schermo
      lcd.setCursor(0, 1);
      lcd.print("Premere bottone ");
    } else if (currentState == ARMED &&
               currentData.distanceCm < thresh_distance_min) {
      lcd.setCursor(0, 0);
      lcd.print("  !! WARNING !! ");
      lcd.setCursor(0, 1);
      lcd.print("   Step back    ");
    } else {
      updateDisplay();
    }
  }
}
