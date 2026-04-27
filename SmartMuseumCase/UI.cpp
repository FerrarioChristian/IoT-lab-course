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
  lcd.print("Light:");
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
    lcd.print("State: ARMED    ");
  } else if (currentState == DISARMED) {
    lcd.print("State: DISARMED ");
  } else {
    lcd.print("!! ALARM !!     ");
  }
}

void drawPageWiFi() {
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("Connected ");
  } else {
    lcd.print("Disconn.  ");
  }

  lcd.setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print(WiFi.localIP().toString());
    lcd.print("     ");
  } else {
    lcd.print("IP: Waiting...  ");
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

// Routine "Bus Recovery" per evitare blocchi dell'I2C dopo un riavvio software
void recoverI2C() {
  pinMode(PIN_I2C_SDA, INPUT_PULLUP);
  pinMode(PIN_I2C_SCL, INPUT_PULLUP);
  delay(10);

  // Se la linea è tenuta giù (stuck), il display stava trasmettendo al momento
  // del reset
  if (digitalRead(PIN_I2C_SDA) == LOW) {
    pinMode(PIN_I2C_SCL, OUTPUT);
    for (int i = 0; i < 9;
         i++) { // Max 9 colpi di clock per svuotare il buffer del PCF8574
      digitalWrite(PIN_I2C_SCL, HIGH);
      delayMicroseconds(20);
      digitalWrite(PIN_I2C_SCL, LOW);
      delayMicroseconds(20);
      if (digitalRead(PIN_I2C_SDA) == HIGH)
        break;
    }
  }
  // Ripristina allo stato neutro
  pinMode(PIN_I2C_SDA, INPUT);
  pinMode(PIN_I2C_SCL, INPUT);
}

void setupUI() {
  // Sblocca forzatamente gli screen I2C bloccati dal soft-reset
  recoverI2C();

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
    addLog("Rotary moved! Value: " + String(encoderCount));
  }

  if (currentMillis - lastUiUpdate >= uiInterval) {
    lastUiUpdate = currentMillis;

    if (currentState == ALARM_ACTIVE) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print(" !! INTRUSION !!");
      lcd.setCursor(0, 1);
      lcd.print("  Press button  ");
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
      lcd.print("  !! ALERT !!   ");
      lcd.setCursor(0, 1);
      lcd.print("High Temperature");
      interrupts();
    } else if (currentData.humidity > thresh_hum_max) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("  !! ALERT !!   ");
      lcd.setCursor(0, 1);
      lcd.print(" High Humidity  ");
      interrupts();
    } else if (currentData.lightLevel > thresh_light_max) {
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("  !! ALERT !!   ");
      lcd.setCursor(0, 1);
      lcd.print("  Excess Light  ");
      interrupts();
    } else {
      updateDisplay();
    }
  }

  // --- LOGICA ATTUATORI (BUZZER) ESEGUITA AD OGNI CICLO ---
  bool isAlarm = (currentState == ALARM_ACTIVE);
  bool isWarning =
      (currentState == ARMED && currentData.distanceCm < thresh_distance_min) ||
      (currentData.temperature > thresh_temp_max) ||
      (currentData.humidity > thresh_hum_max) ||
      (currentData.lightLevel > thresh_light_max);

  // LOGICA BUZZER
  static bool isBuzzing = false;
  static unsigned int currentFreq = 0;

  if (isAlarm) {
    // Sirena bitonale per allarme grave (intrusione)
    unsigned int freq = ((currentMillis / 300) % 2 == 0) ? 1200 : 800;
    if (freq != currentFreq) {
      pinMode(PIN_BUZZER, OUTPUT);
      tone(PIN_BUZZER, freq);
      currentFreq = freq;
    }
    isBuzzing = true;
  } else if (isWarning) {
    // Beep discontinuo per warning (150ms di suono ogni secondo)
    if (currentMillis % 1000 < 150) {
      if (currentFreq != 1000) {
        pinMode(PIN_BUZZER, OUTPUT);
        tone(PIN_BUZZER, 1000);
        currentFreq = 1000;
      }
      isBuzzing = true;
    } else {
      if (isBuzzing) {
        noTone(PIN_BUZZER);
        // FIX ESP8266: La funzione noTone() a volte fallisce nello scollegare
        // completamente il timer hardware dal pin, lasciando un suono fisso.
        // Mettendo il pin in INPUT (Alta Impedenza) lo scolleghiamo fisicamente,
        // garantendo il silenzio totale ed evitando interferenze.
        pinMode(PIN_BUZZER, INPUT);
        isBuzzing = false;
        currentFreq = 0;
      }
    }
  } else {
    if (isBuzzing || currentFreq != 0) {
      noTone(PIN_BUZZER);
      // FIX ESP8266: Isola il pin in alta impedenza per sicurezza
      pinMode(PIN_BUZZER, INPUT);
      isBuzzing = false;
      currentFreq = 0;
    }
  }
}
