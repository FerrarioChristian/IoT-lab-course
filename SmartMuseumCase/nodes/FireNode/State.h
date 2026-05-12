#ifndef STATE_H
#define STATE_H

// ==========================================
// MACCHINA A STATI: STATO DEL SISTEMA
// ==========================================
enum SystemState { ARMED, DISARMED, ALARM_ACTIVE };

extern volatile SystemState currentState;

#endif // STATE_H