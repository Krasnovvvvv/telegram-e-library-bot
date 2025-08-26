#ifndef TG_BOT_FIND_BY_TITLECOMMAND_H
#define TG_BOT_FIND_BY_TITLECOMMAND_H

#pragma once

#include "FindByFieldCommand.h"
#include <sstream>

enum class FindTitleState {
    NONE,
    WAIT_TITLE
};

struct FindTitleSession {
    FindTitleState state = FindTitleState::NONE;
    static constexpr FindTitleState waitState = FindTitleState::WAIT_TITLE;
    int lastBotMsg = 0;
    int64_t userId = 0;
    int topMsgId = 0;
};

class FindByTitleCommand : public FindByFieldCommand<FindTitleSession> {
public:
    FindByTitleCommand(sqlite3* db, TgBot::Bot& bot, YandexDiskClient& yandex)
            : FindByFieldCommand<FindTitleSession>(db, bot, yandex, "title",
                                                   "Введите название книги (например, Занимательная физика):") {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        auto& session = this->sessions[message->from->id];
        session.state = FindTitleSession::waitState;
        session.userId = message->from->id;

        auto topBooks = paginator.getTopBooks(10);
        if (!topBooks.empty()) {
            std::ostringstream oss;
            oss << u8"📚 *ТОП-10 КНИГ:*\n\n";
            int num = 1;
            for (const auto& book : topBooks) {
                oss << num++ << ". " << book.first << " — " << book.second << "\n";
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

#endif // TG_BOT_FIND_BY_TITLECOMMAND_H
