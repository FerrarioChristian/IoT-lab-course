#include "UI.h"
#include "Config.h"
#include "State.h"
#include "WebInterface.h"
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DISPLAY_CHARS 16
#define DISPLAY_LINES 2
#define DISPLAY_ADDR 0x27

LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);

volatile int lastEncoded = 0;
unsigned long lastUiUpdate = 0;
const unsigned long uiInterval = 1000;

// ==========================================
// ISR ROTARY ENCODER
// ==========================================
void ICACHE_RAM_ATTR handleEncoderInterrupt() {
  if (digitalRead(PIN_ENC_DT) == HIGH) {
    encoderCount++;
  } else {
    encoderCount--;
  }
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
    lcd.print("     ");
  } else {
    lcd.print("IP: Attesa...   ");
  }
}

void updateDisplay() {
  noInterrupts();
  lcd.clear();
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
  interrupts();
}

void changePage(int direction) {
  int next = (int)currentPage + direction;
  if (next > 2)
    next = 0;
  if (next < 0)
    next = 2;
  currentPage = (PageState)next;

  lastUiUpdate = 0;
}

// ==========================================
// SETUP & EXECUTIONS
// ==========================================
void setupUI() {
  // I2C - Limite Clock Stretching cruciale per evitare che la libreria I2C
  // finisca in un timeout enorme bloccando il web server quando l'encoder
  // interrompe
  Wire.begin();
  Wire.setClock(100000);
  Wire.setClockStretchLimit(2000);

  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();
  if (error == 0) {
    addLog("LCD found at address 0x27");
    lcd.begin(DISPLAY_CHARS, DISPLAY_LINES);
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
    lcd.print("   Booting... ");
  } else {
    addLog("LCD not found, check I2C Error");
  }

  // TODO: Check if pullup is necessary
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoderInterrupt,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), handleEncoderButton,
                  FALLING);
}

void taskUI() {
  unsigned long currentMillis = millis();

  static int localLastCount = 0;
  if (encoderCount != localLastCount) {
    if (encoderCount > localLastCount)
      changePage(1);
    else
      changePage(-1);
    localLastCount = encoderCount;
    addLog("Rotary Mosso! Valore: " + String(encoderCount));
  }

  if (currentMillis - lastUiUpdate >= uiInterval) {
    lastUiUpdate = currentMillis;

    if (currentState == ALARM_ACTIVE) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("!! INTRUSIONE !!");
      lcd.setCursor(0, 1);
      lcd.print("Premere bottone ");
      interrupts();
    } else if (currentState == ARMED &&
               currentData.distanceCm < thresh_distance_min) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("  !! WARNING !! ");
      lcd.setCursor(0, 1);
      lcd.print("   Step back    ");
      interrupts();
    } else if (currentData.temperature > thresh_temp_max) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("!! ATTENZIONE !!");
      lcd.setCursor(0, 1);
      lcd.print(" Temp eccessiva ");
      interrupts();
    } else if (currentData.humidity > thresh_hum_max) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("!! ATTENZIONE !!");
      lcd.setCursor(0, 1);
      lcd.print("  Umidita alta  ");
      interrupts();
    } else if (currentData.lightLevel > thresh_light_max) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("!! ATTENZIONE !!");
      lcd.setCursor(0, 1);
      lcd.print(" Luce eccessiva ");
      interrupts();
    } else {
      updateDisplay();
    }
  }
}
