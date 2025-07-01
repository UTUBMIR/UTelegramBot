#include "UTelegramBot.h"

const char* BOT_SEND_MESSAGE = "sendMessage";
const char* BOT_SEND_GET_UPDATES = "getUpdates?timeout=20";

// Статичний буфер для відповіді (не локальний!)
static char response[1096];

Bot::Bot(const char* token) {
  strncpy(m_token, token, sizeof(m_token));
  m_token[sizeof(m_token) - 1] = '\0';

  m_client.setInsecure();

  m_host = "api.telegram.org";
  m_port = 443;
  m_chatIdString = "?chat_id=";

  snprintf(m_url, sizeof(m_url), "https://api.telegram.org/bot%s/", m_token);
}

char* Bot::httpGet(const char* url) {
  memset(response, 0, sizeof(response));
  size_t len = 0;

  if (!m_client.connect(m_host, m_port)) {
      strncpy(response, "Connection failed", sizeof(response));
      return response;
  }

  char request[256];
  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: close\r\n\r\n",
           url, m_host);
  m_client.print(request);

  while ((m_client.connected() || m_client.available()) && len < sizeof(response) - 1) {
      if (m_client.available()) {
          response[len++] = m_client.read();
      }
  }

  response[len] = '\0';
  return response;
}

char* Bot::send(const char* method, const char* chatId, const char* text, const char* replyMarkup) {
  //if (!chatId) return (char*)"Error: chatId is NULL";

  char requestUrl[512] = "";
  size_t remaining = sizeof(requestUrl);

  if (chatId)
    snprintf(requestUrl, sizeof(requestUrl), "%s%s%s%s", m_url, method, m_chatIdString, chatId);
  else
    snprintf(requestUrl, sizeof(requestUrl), "%s%s%s", m_url, method, m_chatIdString);
  remaining -= strlen(requestUrl);

  if (text != NULL) {
    strncat(requestUrl, "&text=", remaining);
    remaining = sizeof(requestUrl) - strlen(requestUrl);
    strncat(requestUrl, text, remaining);
    remaining = sizeof(requestUrl) - strlen(requestUrl);
  }

  if (replyMarkup != NULL) {
    strncat(requestUrl, "&reply_markup=", remaining);
    remaining = sizeof(requestUrl) - strlen(requestUrl);
    strncat(requestUrl, replyMarkup, remaining);
  }

  Serial.println(requestUrl);
  return httpGet(requestUrl);
}

char* Bot::sendTextMessage(const char* chatId, const char* text, const char* replyMarkup) {
    return send(BOT_SEND_MESSAGE, chatId, text, replyMarkup);
}

void Bot::setCommand(const char* command, void (*func)(Bot&, const char*)) {
  if (m_commandCount < 10) {
      m_commands[m_commandCount].command = command;
      m_commands[m_commandCount].callback = func;
      ++m_commandCount;
  }
}

void Bot::handleUpdatesTask(void* pvParameters) {
  Bot* self = static_cast<Bot*>(pvParameters);
  self->internalHandleUpdates();
  vTaskDelete(NULL);
}

// Вся логіка оновлень — у звичайній функції
void Bot::internalHandleUpdates() {
  char requestUrl[512];
  Serial.println(m_lastUpdateId);
  snprintf(requestUrl, sizeof(requestUrl), "%s&offset=%lu", BOT_SEND_GET_UPDATES, m_lastUpdateId + 1);

  char* rawResponse = send(requestUrl);
  if (!rawResponse) return;

  char* jsonStart = strchr(rawResponse, '{');
  if (!jsonStart) return;
  Serial.println(jsonStart);
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, jsonStart);
  if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
  }

  if (!doc["ok"]) return;

  JsonArray results = doc["result"].as<JsonArray>();
  for (JsonObject update : results) {
      static char m_lastUpdateIdBuffer[16];
      snprintf(m_lastUpdateIdBuffer, sizeof(m_lastUpdateIdBuffer), "%lu", update["update_id"].as<unsigned long>());

      if (atol(m_lastUpdateIdBuffer) > m_lastUpdateId) {
          m_lastUpdateId = atol(m_lastUpdateIdBuffer);
      }

      if (update.containsKey("message")) {
          const char* text = update["message"]["text"];
          static char chatIdBuffer[32];
          snprintf(chatIdBuffer, sizeof(chatIdBuffer), "%lld", update["message"]["chat"]["id"].as<long long>());
          const char* chatId = chatIdBuffer;

          Serial.println(m_lastUpdateId);
          Serial.println(chatId);

          for (int i = 0; i < m_commandCount; ++i) {
              if (strcmp(text, m_commands[i].command) == 0) {
                  m_commands[i].callback(*this, chatId);
              }
          }
      }
  }
}

void Bot::handleUpdates() {
  xTaskCreate(Bot::handleUpdatesTask, "handleUpdatesTask", 8192, this, 1, NULL);
}
