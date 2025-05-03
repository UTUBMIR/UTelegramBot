#include <WiFi.h>
#include "UTelegramBot.h"

const char* ssid = "HUAWEI-367A";
const char* password = "56061281";
const String token = "7734388562:AAE37siWhffLOirHfRCS5E-O5bTx1ebnaxE";

Bot bot(token);

void startCommand(Bot& b, const String& chatId) {
    b.sendTextMessage(chatId, "Hello");
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");

    bot.setCommand("/start", startCommand);
}

void loop() {
    bot.handleUpdates();
    delay(1000);
}
