#ifndef ACTUATORS_H
#define ACTUATORS_H

/**
 * Inizializza i pin per il Buzzer e il LED RGB.
 */
void setupActuators();

/**
 * Gestisce l'esecuzione di un allarme critico (suono sirena bitonale + LED Rosso lampeggiante).
 */
void playAlarm();

/**
 * Gestisce un allarme di avvertimento (beep + LED Giallo/Arancione).
 */
void playWarning();

/**
 * Muta il suono e spegne o imposta i LED in stato normale (Verde o spento).
 */
void stopActuators();

#endif // ACTUATORS_H