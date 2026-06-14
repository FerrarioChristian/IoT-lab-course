#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

typedef void (*CommandCallback)();
typedef String (*StatusCallback)();

class TelegramManager {
private:
    WiFiClientSecure secured_client;
    UniversalTelegramBot* bot;
    String chatId;
    
    unsigned long bot_lasttime = 0;
    const unsigned long BOT_MTBS = 3000; // Ridotto a 3 secondi per evitare freeze eccessivi sul Master

    // Callbacks
    CommandCallback muteCallback = nullptr;
    CommandCallback armCallback = nullptr;
    CommandCallback disarmCallback = nullptr;
    CommandCallback triggerFireCallback = nullptr;
    CommandCallback triggerImpactCallback = nullptr;
    StatusCallback statusCallback = nullptr;

    void handleNewMessages(int numNewMessages);

public:
    TelegramManager();
    ~TelegramManager();

    void setup(const char* botToken, const char* trustedChatId);
    void loop();
    void sendAlert(String message);

    // Registrazione Callback
    void onMuteCommand(CommandCallback cb) { muteCallback = cb; }
    void onArmCommand(CommandCallback cb) { armCallback = cb; }
    void onDisarmCommand(CommandCallback cb) { disarmCallback = cb; }
    void onTriggerFireCommand(CommandCallback cb) { triggerFireCallback = cb; }
    void onTriggerImpactCommand(CommandCallback cb) { triggerImpactCallback = cb; }
    void onStatusCommand(StatusCallback cb) { statusCallback = cb; }
};

#endif // TELEGRAM_MANAGER_H
