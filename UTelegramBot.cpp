#include "UTelegramBot.h"

Bot::Bot(const String& token) : m_token(token) {
    m_client.setInsecure();
    m_url = "https://api.telegram.org/bot" + token + '/';
}

String Bot::httpGet(const String& url) {
    if (!m_client.connect(m_host.c_str(), m_port)) {
        return "Connection failed";
    }

    m_client.print("GET " + url + " HTTP/1.1\r\n"
                   "Host: " + m_host + "\r\n"
                   "Connection: close\r\n\r\n");

    String response;
    while (m_client.connected() || m_client.available()) {
        if (m_client.available()) {
            response += m_client.readStringUntil('\n');
        }
    }

    return response;
}

String Bot::send(const String& method, const String& chatId, const String& text, const String& replyMarkup) {
    String requestUrl = m_url + method + m_chatIdString + chatId;

    if (!text.isEmpty()) {
        requestUrl += "&text=" + text;
    }
    if (!replyMarkup.isEmpty()) {
        requestUrl += "&reply_markup=" + replyMarkup;
    }

    return httpGet(requestUrl);
}

String Bot::sendTextMessage(const String& chatId, const String& text, const String& replyMarkup) {
    return send(BOT_SEND_MESSAGE, chatId, text, replyMarkup);
}

void Bot::setCommand(const String& command, void (*func)(Bot&, const String&)) {
    if (m_commandCount < 10) {
        m_commands[m_commandCount++] = {command, func};
    }
}

void Bot::handleUpdates() {
    String response = send(BOT_SEND_GET_UPDATES + "&offset=" + String(m_lastUpdateId + 1));

    int jsonStart = response.indexOf('{');
    if (jsonStart == -1) return;
    response = response.substring(jsonStart);

    Serial.println("RAW RESPONSE:");
    Serial.println(response);

    StaticJsonDocument<BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    if (!doc["ok"]) return;

    JsonArray results = doc["result"].as<JsonArray>();
    for (JsonObject update : results) {
        m_lastUpdateId = update["update_id"];
        if (update.containsKey("message")) {
            String text = update["message"]["text"];
            String chatId = update["message"]["chat"]["id"].as<String>();

            for (int i = 0; i < m_commandCount; ++i) {
                if (text == m_commands[i].command) {
                    m_commands[i].callback(*this, chatId);
                }
            }
        }
    }
}
