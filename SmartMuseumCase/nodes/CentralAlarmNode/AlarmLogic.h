#ifndef ALARM_LOGIC_H
#define ALARM_LOGIC_H

#include <Arduino.h>
#include "State.h"

// Forward declarations
class MqttManager;
class TelegramManager;
class InfluxManager;

void initAlarmLogic(MqttManager* mqtt, TelegramManager* tele, InfluxManager* influx);

void checkNodeThresholds(int i, bool &globalWarning);
void muteAllAlarms();
void triggerManualFire();
void triggerManualImpact();
void armAllNodes();
void disarmAllNodes();
String getSystemStatus();

#endif // ALARM_LOGIC_H
