#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * Inizializza comunicazione I2C, lo schermo LCD e gli
 * interrupt per il modulo Rotary Encoder.
 */
void setupDisplay();

/**
 * Mostra un messaggio a schermo bloccante per la configurazione WiFi
 */
void showSetupMessage(const char* apName);

/**
 * Task asincrono da eseguire nel loop per l'aggiornamento continuo
 * dell'LCD e lettura delle variazioni asincrone dell'Encoder.
 */
void taskDisplay();

#endif // DISPLAY_H