#ifndef TG_BOT_FINDCOMMAND_H
#define TG_BOT_FINDCOMMAND_H
#pragma once

#include "SessionCommand.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

enum class FindState {
    NONE,
    WAIT_AUTHOR,
    WAIT_TITLE
};

struct FindSession {
    FindState state = FindState::NONE;
    std::string author;
    int lastBotMsg = 0;
};

class FindCommand : public SessionCommand<FindSession> {
public:
    explicit FindCommand(sqlite3* db_) : db(db_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        int64_t userId = message->from->id;
        auto& session = this->sessions[userId];
        session.state = FindState::WAIT_AUTHOR;
        session.author.clear();
        session.lastBotMsg = bot.getApi().sendMessage(
                message->chat->id,
                "Введите фамилию и инициалы автора книги (например, Дж. К. Роулинг):"
        )->messageId;
    }

protected:
    bool handleSessionMessage(TgBot::Bot& bot,
                              TgBot::Message::Ptr message,
                              FindSession& session) override {
        try { bot.getApi().deleteMessage(message->chat->id, message->messageId); } catch (...) {}
        if (session.lastBotMsg != 0) {
            try { bot.getApi().deleteMessage(message->chat->id, session.lastBotMsg); } catch (...) {}
        }

        auto trim = [](std::string s) {
            s.erase(0, s.find_first_not_of(" \n\r\t"));
            s.erase(s.find_last_not_of(" \n\r\t") + 1);
            return s;
        };

        if (session.state == FindState::WAIT_AUTHOR) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                session.author = input;
                session.state = FindState::WAIT_TITLE;
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id, "Введите название книги:"
                )->messageId;
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректный ввод.\nПопробуйте ещё раз."
                )->messageId;
            }
            return true;
        }

        if (session.state == FindState::WAIT_TITLE) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                doBookSearch(bot, message, session.author, input);
                this->sessions.erase(message->from->id);
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id, "Некорректное название. Попробуйте ещё раз."
                )->messageId;
            }
            return true;
        }

        return false;
    }

private:
    sqlite3* db;

    void doBookSearch(TgBot::Bot& bot,
                      TgBot::Message::Ptr message,
                      const std::string& author,
                      const std::string& title) {
        const char* sql = "SELECT title, author, topic FROM books "
                          "WHERE LOWER(author)=LOWER(?) AND LOWER(title)=LOWER(?) LIMIT 5;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к базе данных.");
            return;
        }
        sqlite3_bind_text(stmt, 1, author.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);

        std::ostringstream resultMsg;
        int cnt = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ++cnt;
            std::string t(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string a(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            std::string topic(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            resultMsg << cnt << ". _" << t << "_ — *" << a
                      << "* (Тема: _" << topic << "_)\n";
        }
        sqlite3_finalize(stmt);

        if (cnt > 0) {
            bot.getApi().sendMessage(message->chat->id, resultMsg.str(), false, 0, nullptr, "Markdown");
        } else {
            bot.getApi().sendMessage(message->chat->id,
                                     "Не найдено совпадений. Проверьте правильность введённых данных.");
        }
    }
};

#endif // TG_BOT_FINDCOMMAND_H
