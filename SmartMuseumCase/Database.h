#ifndef DATABASE_H
#define DATABASE_H

/**
 * Inizializza il client InfluxDB, si occupa del sync dell'orario NTP
 * (necessario per inserimenti timestamp precisi per TimeSeries DB)
 * e controlla lo status della connessione col server locale o cloud.
 */
void setupDatabase();

/**
 * Invia in modo silente (non bloccante) la telemetria periodica
 * ad intervalli regolari dettati dal counter millis.
 */
void taskDatabase();

/**
 * Funzione trigger su chiamata (Push)
 * Da chiamare specificamente quando si avvia un'azione che merita di
 * restare come evento discreto indipendente nel log (Es. Intrusione, Allarme
 * spento)
 */
void logSystemEvent(const char* eventName, const char* details = "");

#endif // DATABASE_H
