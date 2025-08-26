#ifndef TG_BOT_FIND_BY_AUTHORCOMMAND_H
#define TG_BOT_FIND_BY_AUTHORCOMMAND_H

#pragma once

#include "FindByFieldCommand.h"
#include <sstream>

enum class FindAuthorState {
    NONE,
    WAIT_AUTHOR
};

struct FindAuthorSession {
    FindAuthorState state = FindAuthorState::NONE;
    static constexpr FindAuthorState waitState = FindAuthorState::WAIT_AUTHOR;
    int lastBotMsg = 0;
    int64_t userId = 0;
    int topMsgId = 0;
};

class FindByAuthorCommand : public FindByFieldCommand<FindAuthorSession> {
public:
    FindByAuthorCommand(sqlite3* db, TgBot::Bot& bot, YandexDiskClient& yandex)
            : FindByFieldCommand<FindAuthorSession>(db, bot, yandex, "author",
                                                    "Введите фамилию и/или инициалы автора книги (например, Дж. К. Роулинг):") {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        auto& session = this->sessions[message->from->id];
        session.state = FindAuthorSession::waitState;
        session.userId = message->from->id;

        auto topAuthors = paginator.getTopAuthors(10);
        if (!topAuthors.empty()) {
            std::ostringstream oss;
            oss << u8"🔥 *ТОП-10 АВТОРОВ:*\n\n";
            int num = 1;
            for (const auto& author : topAuthors) {
                oss << num++ << ". " << author << "\n";
            }
            session.topMsgId = bot.getApi().sendMessage(
                    message->chat->id,
                    oss.str(),
                    false, 0, nullptr, "Markdown"
            )->messageId;
        } else {
            session.topMsgId = 0;
        }
        session.lastBotMsg = bot.getApi().sendMessage(message->chat->id, prompt)->messageId;
    }

};

#endif // TG_BOT_FIND_BY_AUTHORCOMMAND_H
