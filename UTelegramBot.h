#pragma once
#include <iostream>
#include <string>
#include <cctype>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <locale>
#include <codecvt>

#include <curl/curl.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;


#define BOT_SEND_MESSAGE "sendMessage"
#define BOT_SEND_GET_UPDATES "getUpdates?timeout=20"
#define BOT_INLINE_BUTTONS std::vector<std::vector<std::array<std::string, 2>>>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}


class Bot {
public:
    static std::string wstringToString(const std::wstring& wstr);
    static std::wstring stringToWstring(const std::string& str);
    static std::string HttpGet(const std::string& url);
    static std::string URLEncode(const std::string& text);
    static std::string URLDecode(const std::string& text);
    static std::string CreateInlineKeyboard(const BOT_INLINE_BUTTONS& buttonText);

    std::string callback;

    Bot(const std::string& api): m_api(api), m_url("https://api.telegram.org/bot" + api + '/') {
		if (hasUpdates()) {
			json updates = json::parse(Send(BOT_SEND_GET_UPDATES));
			if (updates["ok"] && !updates["result"].empty()) {
				m_lastUpdateId = updates["result"].back()["update_id"].get<int64_t>();
			}
		}
    }
    Bot(): m_api("") {}

    std::string Send(const std::string& method, const std::string& chatId = "", const std::string& text = "", const std::string& replyMarkup = "");
    std::string SendRaw(const std::string& method, const std::string& chatId, const std::string& addition);
    std::string SendTextMessage(const std::string& chatId, const std::string& text, const std::string& replyMarkup = "");

    // returns true if bot has received updates
    bool hasUpdates();

	// returns json object with updates, all updates after this function will be erased by "offset" (read telegram bot API)
    json getUpdates();

    // set function which will be executed then this command sends
	// first argument is Bot itself, and chatId is the id of chat where command was sent
    void setCommand(const std::string& command, void (*func)(Bot&, const std::string&));

    // set function which will be executed then no commands attached to this comman
    // first argument is Bot itself, and chatId is the id of chat where command was sent
    void setTextHandler(void (*func)(Bot&, const std::string&, const std::string&));
    void setReplyButton(const std::string& replyButton, void (*func)(Bot&, const std::string&));

    void deleteCommand(const std::string& command);
    void deleteTextHandler(const std::string& command);
    void deleteReplyButton(const std::string& replyButton);

    void startLoop();
    void detachLoop();
    void joinLoop();

private:
    long long m_lastUpdateId = 0;
    const std::string m_chatIdString = "?chat_id=";
    std::string m_api;
    std::string m_url;
	std::unordered_map<std::string, void (*)(Bot&, const std::string&)> m_commands;
    std::unordered_map<std::string, void (*)(Bot&, const std::string&)> m_replyButtons;
    void (*m_textHandler)(Bot&, const std::string&, const std::string&) = nullptr;

    std::thread m_loop;
};