#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>

/**
 * Inizializza la connessione WiFi e registra gli endpoint dell'ESP8266WebServer.
 * Si occupa di mappare l'interfaccia HTML ed esponere le REST API in JSON.
 */
void setupWeb();

/**
 * Task continuo per gestire le richieste in arrivo al web server.
 * Questa funzione è bloccante solo per la piccolissima durata della gestione
 * dei client della libreria, essenziale per la reattività del sito.
 */
void handleWebTask();

#endif // WEBINTERFACE_H
