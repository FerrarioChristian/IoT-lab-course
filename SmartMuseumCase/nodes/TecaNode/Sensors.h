#ifndef SENSORS_H
#define SENSORS_H

/**
 * Inizializza i pin hardware dei sensori e imposta gli interrupt
 * per il Knock Sensor e l'ECHO dell'ultrasuoni.
 */
void setupSensors();

/**
 * Funzione asincrona (non bloccante) per campionare i dati dai sensori.
 * Deve essere chiamata nel loop frequentemente.
 */
void taskSensori();

extern volatile bool isMeasuring;

#endif // SENSORS_H
