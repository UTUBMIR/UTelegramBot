#include <WiFi.h>
#include <UTelegramBot.h>

const char* ssid = "wifi_network_name"; // SSID of your wifi network
const char* password = "123123123"; // password of your wifi network
const char* token = "0123456789:AAB11aaAaaaAAaaAAAAA1A-A2aAa1aaaaaA"; // your telegram bot token
Bot bot(token); // creating new bot

void startCommand(Bot& bot, const char* chatId) {
  bot.sendTextMessage(chatId, "Hello!\nto turn on and off the LED use \"/on\" and \"/off\" commands");
}

void setup() {
  WiFi.begin(ssid, password); // connecting to wifi with SSID and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  bot.setCommand("/start", startCommand); // call "startCommand" function when bot recieves "/start"
}

void loop() {
  bot.handleUpdates(); // receive, process and send data
}
