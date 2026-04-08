#include "UI.h"
#include "Config.h"
#include "State.h"
#include "WebInterface.h"
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DISPLAY_CHARS 16  // number of characters on a line
#define DISPLAY_LINES 2   // number of display lines
#define DISPLAY_ADDR 0x27 // display address on I2C bus

// Definiamo il classico schermo LCD 16x2 all'indirizzo hardware base 0x27 (a
// volte può essere 0x3F)
LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);

// Variabili asincrone per decodifica Encoder
volatile int lastEncoded = 0;

unsigned long lastUiUpdate = 0;
const unsigned long uiInterval = 1000;

// ==========================================
// ISR ROTARY ENCODER
// ==========================================
// Integrata perfettamente la logica di lettura dell'encoder del Professore
// all'interno di un interrupt leggerissimo legato SOLO al clock!
void ICACHE_RAM_ATTR handleEncoderInterrupt() {
  static byte encoderClkLast = HIGH;
  byte encoderClkVal = digitalRead(PIN_ENC_CLK);
  if (encoderClkVal != encoderClkLast) {
    if (digitalRead(PIN_ENC_DT) != encoderClkVal) {
      encoderCount--; // Counterclockwise
    } else {
      encoderCount++; // Clockwise
    }
  }
  encoderClkLast = encoderClkVal;
}

// ISR per il pulsante del Rotary Encoder
void ICACHE_RAM_ATTR handleEncoderButton() {
  addLog("Encoder button pressed");
  flagEncoderPressed = true;
}

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
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("Connesso");
  } else {
    lcd.print("Disconn ");
  }

  lcd.setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print(WiFi.localIP().toString());
    lcd.print("     "); // Cancella eventuali ghost char
  } else {
    lcd.print("IP: Attesa...   ");
  }
}

void updateDisplay() {
  noInterrupts(); // PROTEZIONE ATOMICA: Disabilita interrupt sensori
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
  interrupts(); // Riabilita interrupt immediamente
}

// Navigazione pagine
void changePage(int direction) {
  int next = (int)currentPage + direction;
  if (next > 2)
    next = 0; // Wrap around
  if (next < 0)
    next = 2; // Wrap around
  currentPage = (PageState)next;

  noInterrupts();
  lcd.clear(); // Pulisce lo schermo solo al cambio pagina
  interrupts();

  lastUiUpdate = 0; // Forza aggiornamento immediato della vista
}

// ==========================================
// SETUP & EXECUTIONS
// ==========================================
void setupUI() {
  // I2C
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("LCD found at address 0x27");
    lcd.begin(DISPLAY_CHARS, DISPLAY_LINES);
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
    lcd.print("   Booting... ");

  } else {
    Serial.println("LCD not found");
  }

  // Setup ingressi Encoder con attivazione delle pull-up interne per sicurezza
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);

  // Attiviamo la logica professore, ma ad aggancio hardware sul pin CLK
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), handleEncoderButton, FALLING);
}

void taskUI() {
  unsigned long currentMillis = millis();

  // Controllo rotazione controllata 100% in background dall'hardware
  // Reagiamo asincronamente se il target di giri è stato superato. (Uno scatto encoder vale 2 conteggi in questa implementazione)
  static int localLastCount = 0;
  if (encoderCount >= localLastCount + 2 ||
      encoderCount <= localLastCount - 2) {
    if (encoderCount > localLastCount)
      changePage(1);
    else
      changePage(-1);
    localLastCount = encoderCount;
    // Visibile nella pagina web /debug per convalida
    addLog("Paging Hardware! Valore: " + String(encoderCount));
  }

  // // 3. Modifica temporizzata display e priorità degli allarmi a schermo
  if (currentMillis - lastUiUpdate >= uiInterval) {
    lastUiUpdate = currentMillis;

    //   if (currentState == ALARM_ACTIVE) {
    //     noInterrupts();
    //     lcd.setCursor(0, 0);
    //     lcd.print("!! INTRUSIONE !!");
    //     lcd.setCursor(0, 1);
    //     lcd.print("Premere bottone ");
    //     interrupts();
    //   } else if (currentState == ARMED &&
    //              currentData.distanceCm < thresh_distance_min) {
    //     noInterrupts();
    //     lcd.setCursor(0, 0);
    //     lcd.print("  !! WARNING !! ");
    //     lcd.setCursor(0, 1);
    //     lcd.print("   Step back    ");
    //     interrupts();
    //   } else {
    updateDisplay();
    //   }
  }
}
