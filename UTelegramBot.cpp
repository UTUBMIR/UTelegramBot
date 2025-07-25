#include "UTelegramBot.h"

std::string Bot::wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int utf8_len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        -1,
        nullptr,
        0,
        nullptr, nullptr);

    std::string utf8str(utf8_len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], utf8_len, nullptr, nullptr);

    return utf8str;
}

std::wstring Bot::stringToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int wide_len = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        -1,
        nullptr,
        0);

    std::wstring wstr(wide_len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wide_len);

    return wstr;
}

std::string Bot::HttpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            response = "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    else {
        response = "curl_easy_init() failed";
    }

    return response;
}

std::string Bot::URLEncode(const std::string& text) {
    std::string encoded;
    char hex[4];

    for (unsigned char c : text) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        }
        else {
            snprintf(hex, sizeof(hex), "%%%02X", c);
            encoded += hex;
        }
    }
    return encoded;
}

std::string Bot::URLDecode(const std::string& text) {
    std::string decoded;
    char ch;
    int i, j;
    for (i = 0; i < text.length(); i++) {
        if (text[i] == '%' && i + 2 < text.length() &&
            isxdigit(text[i + 1]) && isxdigit(text[i + 2])) {
            std::string hex = text.substr(i + 1, 2);
            ch = static_cast<char>(std::stoi(hex, nullptr, 16));
            decoded += ch;
            i += 2;
        }
        else if (text[i] == '+') {
            decoded += ' ';
        }
        else {
            decoded += text[i];
        }
    }
    return decoded;
}

std::string Bot::SendRaw(const std::string& method, const std::string& chatId, const std::string& addition) {
    std::string requestUrl = m_url + method + m_chatIdString + chatId + addition;

    return URLDecode(HttpGet(requestUrl));
}

std::string Bot::Send(const std::string& method, const std::string& chatId, const std::string& text, const std::string& replyMarkup) {
    std::string addition;
    addition.reserve(12);
    
    if (!text.empty()) {
        addition += "&text=" + URLEncode(text);
    }

    if (!replyMarkup.empty()) {
        addition += "&reply_markup=" + URLEncode(replyMarkup);
    }

    return SendRaw(method, chatId, addition);
}


std::string Bot::SendTextMessage(const std::string& chatId, const std::string& text, const std::string& replyMarkup) {
    return Send(BOT_SEND_MESSAGE, chatId, text, replyMarkup);
}

bool Bot::hasUpdates() {
    std::string updatesRaw = Send(BOT_SEND_GET_UPDATES);

    json updates;
    try {
        updates = json::parse(updatesRaw);
    }
    catch (...) {
        curl_version_info_data* vinfo = curl_version_info(CURLVERSION_NOW);
        std::cout << "Protocols supported:\n";
        for (auto p = vinfo->protocols; *p; ++p) {
            std::cout << " - " << *p << std::endl;
        }

        std::cerr << "Error JSON parsing\n";
        std::cout << updatesRaw << "\n";
        return false;
    }

    return updates["ok"] && !updates["result"].empty();
}

json Bot::getUpdates() {
    json updates = json::parse(Send(BOT_SEND_GET_UPDATES + std::string("&offset=") + std::to_string(m_lastUpdateId + 1)));
    if (updates["ok"] && !updates["result"].empty()) {
        m_lastUpdateId = updates["result"].back()["update_id"].get<int64_t>();
    }
	return updates;
}

void Bot::setCommand(const std::string& command, void(*func)(Bot&, const std::string&)) {
    m_commands[command] = func;
}

void Bot::setTextHandler(void(*func)(Bot&, const std::string&, const std::string&)) {
    m_textHandler = func;
}

void Bot::setReplyButton(const std::string& replyButton, void(*func)(Bot&, const std::string&)) {
    m_replyButtons[replyButton] = func;
}

void Bot::deleteCommand(const std::string& command) {
    m_commands.erase(command);
}

void Bot::deleteTextHandler(const std::string& command) {
    m_textHandler = nullptr;
}

void Bot::deleteReplyButton(const std::string& replyButton) {
	m_replyButtons.erase(replyButton);
}

void Bot::startLoop() {
	m_loop = std::thread([&]() {
		while (true) {
			if (hasUpdates()) {
				json updates = getUpdates();
				for (auto& update : updates["result"]) {
                    if (update.contains("message") && update["message"].is_object()) {
                        if (update["message"].contains("text") && update["message"]["text"].is_string()) {
                            std::string command = update["message"]["text"];
                            if (m_commands.find(command) != m_commands.end()) {
                                m_commands[command](*this, to_string(update["message"]["chat"]["id"]));
                            }
                            else {
                                if (m_textHandler != nullptr)
                                    m_textHandler(*this, command, to_string(update["message"]["chat"]["id"]));
                            }
                        }
                    }
                    if (update.contains("callback_query") && update["callback_query"].is_object()) {
                        if (update["callback_query"].contains("data") && update["callback_query"]["data"].is_string()) {
                            std::string callbackData = update["callback_query"]["data"];
                            if (m_replyButtons.find(callbackData) != m_replyButtons.end()) {
                                callback = callbackData;
                                m_replyButtons[callbackData](*this, to_string(update["callback_query"]["message"]["chat"]["id"]));
                            }
                        }
                    }
				}
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		});
}

void Bot::detachLoop() {
	m_loop.detach();
}

void Bot::joinLoop() {
	m_loop.join();
}

std::string Bot::CreateInlineKeyboard(const BOT_INLINE_BUTTONS& buttonText) {
    std::string inlineKeyboard = "{\"inline_keyboard\":[";

    for (size_t row = 0; row < buttonText.size(); ++row) {
        inlineKeyboard += "[";
        for (size_t col = 0; col < buttonText[row].size(); ++col) {
            inlineKeyboard += "{\"text\":\"" + buttonText[row][col][0] + "\",\"callback_data\":\"" + Bot::URLEncode(buttonText[row][col][1]) + "\"}";

            if (col < buttonText[row].size() - 1) {
                inlineKeyboard += ",";
            }
        }
        inlineKeyboard += "]";
        if (row < buttonText.size() - 1) {
            inlineKeyboard += ",";
        }
    }

    inlineKeyboard += "]}";

    return inlineKeyboard;
}