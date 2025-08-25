#ifndef TG_BOT_FIND_BY_FIELDCOMMAND_H
#define TG_BOT_FIND_BY_FIELDCOMMAND_H

#pragma once

#include "SessionCommand.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

template <typename SessionType>
class FindByFieldCommand : public SessionCommand<SessionType> {
public:
    FindByFieldCommand(sqlite3* db_, const char* field_, const std::string& prompt_)
            : db(db_), field(field_), prompt(prompt_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        int64_t userId = message->from->id;
        auto& session = this->sessions[userId];
        session.state = SessionType::waitState;
        session.lastBotMsg = bot.getApi().sendMessage(message->chat->id, prompt)->messageId;
    }

protected:
    bool handleSessionMessage(TgBot::Bot& bot, TgBot::Message::Ptr message, SessionType& session) override {
        try { bot.getApi().deleteMessage(message->chat->id, message->messageId); } catch (...) {}
        if (session.lastBotMsg != 0) {
            try { bot.getApi().deleteMessage(message->chat->id, session.lastBotMsg); } catch (...) {}
        }

        auto trim = [](const std::string& s) -> std::string {
            size_t start = s.find_first_not_of(" \n\r\t");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \n\r\t");
            return s.substr(start, end - start + 1);
        };

        if (session.state == SessionType::waitState) {
            std::string input = trim(message->text);
            if (!input.empty()) {
                doBookSearch(bot, message, input);
                this->sessions.erase(message->from->id);
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
    const char* field;
    std::string prompt;

    void doBookSearch(TgBot::Bot& bot, TgBot::Message::Ptr message, const std::string& searchStr) {
        // Для полной поддержки Unicode регистра сейчас делаем поиск без LOWER,
        // параметр передаем как есть (с учетом, что совпадения по регистру могут быть чувствительны)
        std::ostringstream sql;
        sql << "SELECT title, author, topic FROM books WHERE " << field << " LIKE ? LIMIT 5;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к базе данных.");
            return;
        }

        std::string pattern = "%" + searchStr + "%";
        sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

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
            bot.getApi().sendMessage(message->chat->id,
                                     resultMsg.str(),
                                     false,
                                     0,
                                     nullptr,
                                     "Markdown");
        } else {
            bot.getApi().sendMessage(message->chat->id, "Не найдено совпадений.");
        }
    }
};

#endif // TG_BOT_FIND_BY_FIELDCOMMAND_H
