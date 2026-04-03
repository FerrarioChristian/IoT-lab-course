#ifndef UI_H
#define UI_H

#include <Arduino.h>

/**
 * Inizializza comunicazione I2C, lo schermo LCD, i PIN dei LED e gli
 * interrupt per il modulo Rotary Encoder.
 */
void setupUI();

/**
 * Task asincrono da eseguire nel loop per l'aggiornamento continuo
 * dell'LCD, lo stato dei LED e lettura delle variazioni asincrone dell'Encoder.
 */
void taskUI();

#endif // UI_H
