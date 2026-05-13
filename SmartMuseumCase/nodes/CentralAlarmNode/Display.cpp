#include "Display.h"
#include "Config.h"
#include "State.h"
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DISPLAY_CHARS 16
#define DISPLAY_LINES 2
#define DISPLAY_ADDR 0x27

LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);

unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 1000;

// ==========================================
// ISR ROTARY ENCODER
// ==========================================
void ICACHE_RAM_ATTR handleEncoderInterrupt() {
  if (digitalRead(PIN_ENC_DT) == HIGH) {
    encoderCount--;
  } else {
    encoderCount++;
  }
}

void ICACHE_RAM_ATTR handleEncoderButton() {
  Serial.println("Encoder button pressed");
  flagEncoderPressed = true;
}

// ==========================================
// FUNZIONI HELPERS LCD
// ==========================================
void updateDisplay() {
  noInterrupts();
  lcd.clear();
  switch (currentPage) {
  case STATE_OVERVIEW:
    lcd.setCursor(0, 0);
    lcd.print("Nodes: ");
    lcd.print(activeNodeCount);
    lcd.setCursor(0, 1);
    if (WiFi.status() == WL_CONNECTED) {
      lcd.print(WiFi.localIP().toString());
    } else {
      lcd.print("No WiFi");
    }
    break;
  case STATE_NODE_LIST:
    lcd.setCursor(0, 0);
    lcd.print("Select Node:");
    lcd.setCursor(0, 1);
    if (activeNodeCount > 0) {
      lcd.print("> ");
      lcd.print(nodeRegistry[selectedNodeIndex].id);
    } else {
      lcd.print("No nodes");
    }
    break;
  case STATE_NODE_DETAIL_1:
    if (activeNodeCount > 0) {
      lcd.setCursor(0, 0);
      lcd.print(nodeRegistry[selectedNodeIndex].id);
      lcd.print(" (1/2) ");
      lcd.setCursor(0, 1);
      
      if (nodeRegistry[selectedNodeIndex].capabilities.hasTemperature) {
        lcd.print("T:");
        lcd.print(nodeRegistry[selectedNodeIndex].data.temperature, 1);
        lcd.print(" ");
      }
      if (nodeRegistry[selectedNodeIndex].capabilities.hasHumidity) {
        lcd.print("H:");
        lcd.print(nodeRegistry[selectedNodeIndex].data.humidity, 0);
      }
      if (!nodeRegistry[selectedNodeIndex].capabilities.hasTemperature && !nodeRegistry[selectedNodeIndex].capabilities.hasHumidity) {
        lcd.print("N/A");
      }
      lcd.print("       ");
    }
    break;
  case STATE_NODE_DETAIL_2:
    if (activeNodeCount > 0) {
      lcd.setCursor(0, 0);
      lcd.print(nodeRegistry[selectedNodeIndex].id);
      lcd.print(" (2/2) ");
      lcd.setCursor(0, 1);
      
      if (nodeRegistry[selectedNodeIndex].capabilities.hasFlame) {
        lcd.print("FLM:");
        lcd.print(nodeRegistry[selectedNodeIndex].data.flameAnalog);
      } else {
        if (nodeRegistry[selectedNodeIndex].capabilities.hasLight) {
          lcd.print("L:");
          lcd.print(nodeRegistry[selectedNodeIndex].data.lightLevel);
          lcd.print(" ");
        }
        if (nodeRegistry[selectedNodeIndex].capabilities.hasDistance) {
          lcd.print("D:");
          float d = nodeRegistry[selectedNodeIndex].data.distanceCm;
          if (d >= 999.0) lcd.print(">400");
          else lcd.print(d, 1);
        }
        if (!nodeRegistry[selectedNodeIndex].capabilities.hasLight && !nodeRegistry[selectedNodeIndex].capabilities.hasDistance) {
          lcd.print("N/A");
        }
      }
      lcd.print("      ");
    }
    break;
  }
  interrupts();
}

void changePage(int direction) {
  if (activeNodeCount > 0) {
    if (currentPage == STATE_NODE_LIST) {
      selectedNodeIndex += direction;
      if (selectedNodeIndex >= activeNodeCount)
        selectedNodeIndex = 0;
      if (selectedNodeIndex < 0)
        selectedNodeIndex = activeNodeCount - 1;
    } else if (currentPage == STATE_NODE_DETAIL_1 ||
               currentPage == STATE_NODE_DETAIL_2) {
      if (currentPage == STATE_NODE_DETAIL_1 && direction > 0)
        currentPage = STATE_NODE_DETAIL_2;
      else if (currentPage == STATE_NODE_DETAIL_2 && direction < 0)
        currentPage = STATE_NODE_DETAIL_1;
    }
  }
  lastDisplayUpdate = 0;
}

// ==========================================
// SETUP & EXECUTIONS
// ==========================================

void recoverI2C() {
  pinMode(PIN_I2C_SDA, INPUT_PULLUP);
  pinMode(PIN_I2C_SCL, INPUT_PULLUP);
  delay(10);

  if (digitalRead(PIN_I2C_SDA) == LOW) {
    pinMode(PIN_I2C_SCL, OUTPUT);
    for (int i = 0; i < 9; i++) {
      digitalWrite(PIN_I2C_SCL, HIGH);
      delayMicroseconds(20);
      digitalWrite(PIN_I2C_SCL, LOW);
      delayMicroseconds(20);
      if (digitalRead(PIN_I2C_SDA) == HIGH)
        break;
    }
  }
  pinMode(PIN_I2C_SDA, INPUT);
  pinMode(PIN_I2C_SCL, INPUT);
}

void setupDisplay() {
  recoverI2C();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);
  Wire.setClockStretchLimit(2000);

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
    Serial.println("LCD not found, check I2C Error");
  }

  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoderInterrupt,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), handleEncoderButton,
                  FALLING);
}

void showSetupMessage(const char *apName) {
  noInterrupts();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect to WiFi:");
  lcd.setCursor(0, 1);
  lcd.print(apName);
  interrupts();
}

void taskDisplay() {
  unsigned long currentMillis = millis();

  static int localLastCount = 0;
  if (encoderCount != localLastCount) {
    if (encoderCount > localLastCount)
      changePage(1);
    else
      changePage(-1);
    localLastCount = encoderCount;
    Serial.println("Rotary moved! Index: " + String(selectedNodeIndex));
  }

  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = currentMillis;

    if (currentState == ALARM_ACTIVE) {
      String alarmingNode = "Unknown";
      String reason = "IMPACT";
      for (int i = 0; i < activeNodeCount; i++) {
        if (nodeRegistry[i].state == ALARM_ACTIVE) {
          alarmingNode = nodeRegistry[i].id;
          if (nodeRegistry[i].alarmReason != "") {
             reason = nodeRegistry[i].alarmReason;
          }
        }
      }
      noInterrupts();
      lcd.setCursor(0, 0);
      lcd.print("! ALARM: ");
      lcd.print(reason);
      lcd.print("      "); // Pad to clear line
      lcd.setCursor(0, 1);
      lcd.print(alarmingNode + "                "); // Pad to clear line
      interrupts();
    } else {
      // Check if any node is in warning state
      bool isWarning = false;
      String warningNode = "";
      String warningReason = "";
      for (int i = 0; i < activeNodeCount; i++) {
        if (nodeRegistry[i].activeWarning) {
          isWarning = true;
          warningNode = nodeRegistry[i].id;
          warningReason = "PROXIMITY";
          break;
        } else if (nodeRegistry[i].data.temperature >
                   nodeRegistry[i].settings.tempMax) {
          isWarning = true;
          warningNode = nodeRegistry[i].id;
          warningReason = "HI-TEMP";
          break;
        } else if (nodeRegistry[i].data.humidity >
                   nodeRegistry[i].settings.humMax) {
          isWarning = true;
          warningNode = nodeRegistry[i].id;
          warningReason = "HI-HUM";
          break;
        } else if (nodeRegistry[i].data.lightLevel >
                   nodeRegistry[i].settings.lightMax) {
          isWarning = true;
          warningNode = nodeRegistry[i].id;
          warningReason = "HI-LIGHT";
          break;
        }
      }

      if (isWarning) {
        noInterrupts();
        lcd.setCursor(0, 0);
        lcd.print("! WARN: ");
        lcd.print(warningReason);
        lcd.print("        "); // Pad to clear line
        lcd.setCursor(0, 1);
        lcd.print(warningNode + "                "); // Pad to clear line
        interrupts();
      } else {
        updateDisplay();
      }
    }
  }
}