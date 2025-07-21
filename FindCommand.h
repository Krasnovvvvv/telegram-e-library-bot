#ifndef TG_BOT_FINDCOMMAND_H
#define TG_BOT_FINDCOMMAND_H
#pragma once

#include <tgbot/tgbot.h>
#include "ICommand.h"
#include <sqlite3.h>
#include <unordered_map>
#include <regex>
#include <sstream>

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

class FindCommand : public ICommand {
public:
    explicit FindCommand(sqlite3* db_) : db(db_) {}

    // user_id to session
    std::unordered_map<int64_t, FindSession> sessions;
    sqlite3* db;

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        int64_t userId = message->from->id;
        auto& session = sessions[userId];
        session.state = FindState::WAIT_AUTHOR;
        session.author.clear();
        session.lastBotMsg = bot.getApi().sendMessage(
                message->chat->id,
                "Введите фамилию и инициалы автора книги (например, Роулинг Дж. К.):\n"
                "Разрешены только буквы, пробелы и точка (например, для инициалов)."
        )->messageId;
    }

    void handleMessage(TgBot::Bot& bot, TgBot::Message::Ptr message) {
        int64_t userId = message->from->id;
        auto it = sessions.find(userId);
        if (it == sessions.end()) return; // не в поиске

        FindSession& session = it->second;

        try { bot.getApi().deleteMessage(message->chat->id, message->messageId); } catch (...) {}

        if (session.lastBotMsg != 0) {
            try { bot.getApi().deleteMessage(message->chat->id, session.lastBotMsg); } catch (...) {}
        }

        if (session.state == FindState::WAIT_AUTHOR) {
            static const std::regex authorRegex(R"(^[А-ЯЁа-яёA-Za-z.\s]+$)");
            if (std::regex_match(message->text, authorRegex)) {
                session.author = message->text;
                session.state = FindState::WAIT_TITLE;
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Введите название книги:"
                )->messageId;
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректный ввод.\n"
                        "Пожалуйста, введите фамилию и инициалы автора (только буквы, пробелы и точка). Попробуйте ещё раз."
                )->messageId;
            }
            return;
        }

        if (session.state == FindState::WAIT_TITLE) {
            static const std::regex titleRegex(R"(^[А-Яа-яA-Za-z0-9\s\-\.\:\,]+$)");
            if (std::regex_match(message->text, titleRegex)) {
                std::string author = session.author;
                std::string title = message->text;
                doBookSearch(bot, message, author, title);
                sessions.erase(userId); // заканчиваем диалог
            } else {
                session.lastBotMsg = bot.getApi().sendMessage(
                        message->chat->id,
                        "Некорректное название.\n"
                        "Допустимы буквы, цифры, пробелы, дефис, точка, двоеточие, запятая. Попробуйте ещё раз."
                )->messageId;
            }
            return;
        }
    }

private:
    // Поиск книги в базе после ввода данных
    void doBookSearch(TgBot::Bot& bot, TgBot::Message::Ptr message, const std::string& author, const std::string& title) {
        const char* sql = "SELECT title, author, topic FROM books WHERE LOWER(author)=LOWER(?) AND LOWER(title)=LOWER(?) LIMIT 5;";
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
            resultMsg << cnt << ". _" << t << "_ — *" << a << "* (Тема: _" << topic << "_)\n";
        }
        sqlite3_finalize(stmt);

        if (cnt > 0) {
            bot.getApi().sendMessage(message->chat->id, resultMsg.str(), false, 0, nullptr, "Markdown");
        } else {
            bot.getApi().sendMessage(
                    message->chat->id,
                    "Не найдено совпадений.\n"
                    "Проверьте правильность введённых фамилии/инициалов и названия книги."
            );
        }
    }
};

#endif // TG_BOT_FINDCOMMAND_H
