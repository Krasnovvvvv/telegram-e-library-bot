#ifndef TG_BOT_FINDBYFIELDCOMMAND_H
#define TG_BOT_FINDBYFIELDCOMMAND_H

#pragma once

#include "SessionCommand.h"
#include "BookListPaginator.h"
#include "YandexDiskClient.h"

#include <string>
#include <vector>

template<typename SessionType>
class FindByFieldCommand : public SessionCommand<SessionType> {
public:
    FindByFieldCommand(sqlite3* db_, TgBot::Bot& bot_, YandexDiskClient& yandex_,
                       const std::string& fieldName_, const std::string& prompt_)
            : db(db_), bot(bot_), yandex(yandex_), paginator(db_, bot_, yandex_), fieldName(fieldName_), prompt(prompt_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        auto& session = this->sessions[message->from->id];
        session.state = SessionType::waitState;
        session.userId = message->from->id;
        session.lastBotMsg = bot.getApi().sendMessage(message->chat->id, prompt)->messageId;
    }

protected:
    bool handleSessionMessage(TgBot::Bot& bot, TgBot::Message::Ptr message, SessionType& session) override {
        safeDeleteMessage(bot, message->chat->id, message->messageId);
        if (session.lastBotMsg != 0)
            safeDeleteMessage(bot, message->chat->id, session.lastBotMsg);

        auto trim = [](const std::string& s) -> std::string {
            size_t start = s.find_first_not_of(" \n\r\t");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \n\r\t");
            return s.substr(start, end - start + 1);
        };

        if (session.state == SessionType::waitState) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                std::string whereClause = fieldName + " LIKE ?";
                std::vector<std::string> params = {"%" + input + "%"};
                paginator.setUserPage(session.userId, 0);
                paginator.sendPage(message->chat->id, session.userId, whereClause, params);
                this->sessions.erase(session.userId);
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(message->chat->id,
                                                              "Некорректный ввод. Попробуйте ещё раз.")->messageId;
            }
            return true;
        }
        return false;
    }

private:
    sqlite3* db;
    TgBot::Bot& bot;
    YandexDiskClient& yandex;
    BookListPaginator paginator;
    std::string fieldName;
    std::string prompt;

    void safeDeleteMessage(TgBot::Bot& bot, int64_t chatId, int msgId) {
        try { bot.getApi().deleteMessage(chatId, msgId); } catch (...) {}
    }
};

#endif // TG_BOT_FINDBYFIELDCOMMAND_H
