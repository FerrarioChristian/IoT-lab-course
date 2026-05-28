#include "TelegramManager.h"

TelegramManager::TelegramManager() {
    bot = nullptr;
}

TelegramManager::~TelegramManager() {
    if (bot) delete bot;
}

void TelegramManager::setup(const char* botToken, const char* trustedChatId) {
    this->chatId = String(trustedChatId);

    // Configura il client HTTPS per saltare la verifica del certificato SSL (per semplicità su ESP8266)
    secured_client.setInsecure();
    
    bot = new UniversalTelegramBot(botToken, secured_client);
    
    Serial.println("TelegramManager: Inizializzato.");
}

void TelegramManager::sendAlert(String message) {
    if (bot && chatId != "") {
        bot->sendMessage(chatId, message, "");
        Serial.println("Telegram Alert Inviato: " + message);
    }
}

void TelegramManager::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = bot->messages[i].chat_id;
        String text = bot->messages[i].text;

        // Sicurezza: Ignora messaggi da chat ID non autorizzati
        if (chat_id != this->chatId) {
            bot->sendMessage(chat_id, "Accesso Negato. Non sei autorizzato a comunicare con questo sistema.", "");
            continue;
        }

        Serial.println("TelegramManager Ricevuto Comando: " + text);

        if (text == "/mute") {
            if (muteCallback) muteCallback();
            bot->sendMessage(chat_id, "🔇 Tutti gli allarmi in corso sono stati silenziati.", "");
        } 
        else if (text == "/arm") {
            if (armCallback) armCallback();
            bot->sendMessage(chat_id, "🛡️ Sistema inserito (ARMED). Sensori attivi.", "");
        } 
        else if (text == "/disarm") {
            if (disarmCallback) disarmCallback();
            bot->sendMessage(chat_id, "🔓 Sistema disinserito (DISARMED). Allarmi disabilitati.", "");
        } 
        else if (text == "/status") {
            if (statusCallback) {
                String statusMsg = statusCallback();
                bot->sendMessage(chat_id, statusMsg, "");
            } else {
                bot->sendMessage(chat_id, "Stato sconosciuto.", "");
            }
        } 
        else if (text == "/trigger_fire") {
            if (triggerFireCallback) triggerFireCallback();
            bot->sendMessage(chat_id, "🔥 SIMULAZIONE ALLARME INCENDIO attivata!", "");
        }
        else if (text == "/trigger_impact") {
            if (triggerImpactCallback) triggerImpactCallback();
            bot->sendMessage(chat_id, "🚨 SIMULAZIONE ALLARME INTRUSIONE attivata!", "");
        }
        else if (text == "/help" || text == "/start") {
            String helpMsg = "Benvenuto nel sistema Smart Museum!\n\n";
            helpMsg += "Comandi disponibili:\n";
            helpMsg += "🚨 /status - Controlla quanti nodi sono connessi e lo stato del sistema\n";
            helpMsg += "🛡️ /arm - Inserisce l'allarme e attiva i sensori\n";
            helpMsg += "🔓 /disarm - Disinserisce l'allarme per la manutenzione\n";
            helpMsg += "🔇 /mute - Silenzia immediatamente gli allarmi in corso in caso di falso allarme\n";
            helpMsg += "\nTest e Simulazioni:\n";
            helpMsg += "🔥 /trigger_fire - Simula un allarme antincendio\n";
            helpMsg += "🚨 /trigger_impact - Simula un'intrusione in una teca\n";
            helpMsg += "\n❓ /help - Mostra questo messaggio";
            bot->sendMessage(chat_id, helpMsg, "");
        }
        else {
            bot->sendMessage(chat_id, "Comando non riconosciuto. Usa /help per la lista dei comandi.", "");
        }
    }
}

void TelegramManager::loop() {
    if (!bot) return;

    if (millis() - bot_lasttime > BOT_MTBS) {
        int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

        while (numNewMessages) {
            Serial.println("TelegramManager: Trovati " + String(numNewMessages) + " nuovi messaggi.");
            handleNewMessages(numNewMessages);
            numNewMessages = bot->getUpdates(bot->last_message_received + 1);
        }

        bot_lasttime = millis();
    }
}
