#ifndef TG_BOT_FIND_BY_AUTHORCOMMAND_H
#define TG_BOT_FIND_BY_AUTHORCOMMAND_H

#pragma once

#include "SessionCommand.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

enum class FindAuthorState {
    NONE,
    WAIT_AUTHOR
};

struct FindAuthorSession {
    FindAuthorState state = FindAuthorState::NONE;
    std::string author;
    int lastBotMsg = 0;
};

class FindByAuthorCommand : public SessionCommand<FindAuthorSession> {
public:
    explicit FindByAuthorCommand(sqlite3* db_) : db(db_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        int64_t userId = message->from->id;
        auto& session = this->sessions[userId];
        session.state = FindAuthorState::WAIT_AUTHOR;
        session.author.clear();
        session.lastBotMsg = bot.getApi().sendMessage(
                message->chat->id,
                "Введите фамилию и инициалы автора книги (например, Дж. К. Роулинг):"
        )->messageId;
    }

protected:
    bool handleSessionMessage(TgBot::Bot& bot,
                              TgBot::Message::Ptr message,
                              FindAuthorSession& session) override {
        try { bot.getApi().deleteMessage(message->chat->id, message->messageId); } catch (...) {}
        if (session.lastBotMsg != 0) {
            try { bot.getApi().deleteMessage(message->chat->id, session.lastBotMsg); } catch (...) {}
        }

        auto trim = [](const std::string& s) {
            size_t start = s.find_first_not_of(" \n\r\t");
            size_t end = s.find_last_not_of(" \n\r\t");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };

        if (session.state == FindAuthorState::WAIT_AUTHOR) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                doBookSearchByAuthor(bot, message, input);
                this->sessions.erase(message->from->id);
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректный ввод. Попробуйте ещё раз."
                )->messageId;
            }
            return true;
        }
        return false;
    }

private:
    sqlite3* db;

    void doBookSearchByAuthor(TgBot::Bot& bot,
                              TgBot::Message::Ptr message,
                              const std::string& author) {
        const char* sql = "SELECT title, author, topic FROM books "
                          "WHERE LOWER(author) LIKE LOWER(?) LIMIT 5;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к базе данных.");
            return;
        }

        std::string authorPattern = "%" + author + "%";
        sqlite3_bind_text(stmt, 1, authorPattern.c_str(), -1, SQLITE_TRANSIENT);

        std::ostringstream resultMsg;
        int cnt = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ++cnt;
            std::string t(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string a(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            std::string topic(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            resultMsg << cnt << ". _" << t << "_ — *" << a << "* (Тема: _" << topic << "_)\n";
        }
        sqlite3_finalize(stmt);

        if (cnt > 0) {
            bot.getApi().sendMessage(message->chat->id, resultMsg.str(), false, 0, nullptr, "Markdown");
        } else {
            bot.getApi().sendMessage(message->chat->id,
                                     "Не найдено совпадений по автору.");
        }
    }
};

#endif // TG_BOT_FIND_BY_AUTHORCOMMAND_H
