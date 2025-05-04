#ifndef UTELEGRAMBOT_H
#define UTELEGRAMBOT_H

#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define BUFFER_SIZE 2048  // для JSON-десеріалізації

class Bot {
public:
    Bot(const char* token);

    char* sendTextMessage(const char* chatId, const char* text, const char* replyMarkup = nullptr);
    void setCommand(const char* command, void (*func)(Bot&, const char*));
    void handleUpdates();

private:
    char* send(const char* method, const char* chatId, const char* text, const char* replyMarkup = nullptr);
    char* httpGet(const char* url);

private:
    WiFiClientSecure m_client;

    char m_token[64];
    char m_url[128];
    const char* m_host;
    uint16_t m_port;
    const char* m_chatIdString;

    unsigned long m_lastUpdateId = 0;

    struct Command {
        const char* command;
        void (*callback)(Bot&, const char*);
    };

    Command m_commands[10];
    int m_commandCount = 0;
};

#endif
