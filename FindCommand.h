#ifndef TG_BOT_FINDCOMMAND_H
#define TG_BOT_FINDCOMMAND_H

#pragma once

#include "SessionCommand.h"
#include "BookListPaginator.h"
#include "YandexDiskClient.h"

#include <sqlite3.h>

enum class FindState {
    NONE,
    WAIT_AUTHOR,
    WAIT_TITLE
};

struct FindSession {
    FindState state = FindState::NONE;
    std::string author;
    int lastBotMsg = 0;
    int64_t userId = 0;
};

class FindCommand : public SessionCommand<FindSession> {
public:
    FindCommand(sqlite3* db_, TgBot::Bot& bot_, YandexDiskClient& yandex_)
            : db(db_), bot(bot_), yandex(yandex_), paginator(db_, bot_, yandex_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        auto& session = this->sessions[message->from->id];
        session.state = FindState::WAIT_AUTHOR;
        session.author.clear();
        session.userId = message->from->id;
        session.lastBotMsg = bot.getApi().sendMessage(
                message->chat->id,
                "Введите фамилию и инициалы автора книги (например, Дж. К. Роулинг):"
        )->messageId;
    }

protected:
    bool handleSessionMessage(TgBot::Bot& bot, TgBot::Message::Ptr message, FindSession& session) override {
        safeDeleteMessage(bot, message->chat->id, message->messageId);
        if (session.lastBotMsg != 0)
            safeDeleteMessage(bot, message->chat->id, session.lastBotMsg);

        auto trim = [](const std::string& s) -> std::string {
            size_t start = s.find_first_not_of(" \n\r\t");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \n\r\t");
            return s.substr(start, end - start + 1);
        };

        if (session.state == FindState::WAIT_AUTHOR) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                session.author = input;
                session.state = FindState::WAIT_TITLE;
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Введите название книги:"
                )->messageId;
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректный ввод. Попробуйте ещё раз."
                )->messageId;
            }
            return true;
        }

        if (session.state == FindState::WAIT_TITLE) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                std::string whereClause = "author LIKE ? AND title LIKE ?";
                std::vector<std::string> params = {"%" + session.author + "%", "%" + input + "%"};
                paginator.setUserPage(session.userId, 0);
                paginator.sendPage(message->chat->id, session.userId, whereClause, params);
                this->sessions.erase(session.userId);
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректное название. Попробуйте ещё раз."
                )->messageId;
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

    void safeDeleteMessage(TgBot::Bot& bot, int64_t chatId, int msgId) {
        try { bot.getApi().deleteMessage(chatId, msgId); }
        catch (...) {}
    }
};

#endif // TG_BOT_FINDCOMMAND_H
