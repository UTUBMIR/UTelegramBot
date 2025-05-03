#pragma once
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const String BOT_SEND_MESSAGE("sendMessage");
const String BOT_SEND_GET_UPDATES("getUpdates?timeout=20");

class Bot {
public:
    String httpGet(const String& url);
    Bot(const String& token);
    String send(const String& method, const String& chatId = "", const String& text = "", const String& replyMarkup = "");
    String sendTextMessage(const String& chatId, const String& text, const String& replyMarkup = "");
    void handleUpdates();
    void setCommand(const String& command, void (*func)(Bot&, const String&));

private:
    String m_token;
    String m_host = "api.telegram.org";
    int m_port = 443;
    String m_url;
    WiFiClientSecure m_client;
    unsigned long m_lastUpdateId = 0;
    static const int BUFFER_SIZE = 4048;

    struct Command {
        String command;
        void (*callback)(Bot&, const String&);
    };

    Command m_commands[10];
    int m_commandCount = 0;

    const String m_chatIdString = "?chat_id=";
};
