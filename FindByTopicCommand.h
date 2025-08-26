#ifndef TG_BOT_FIND_BY_TOPICCOMMAND_H
#define TG_BOT_FIND_BY_TOPICCOMMAND_H

#pragma once

#include "FindByFieldCommand.h"
#include <sstream>

enum class FindTopicState {
    NONE,
    WAIT_TOPIC
};

struct FindTopicSession {
    FindTopicState state = FindTopicState::NONE;
    static constexpr FindTopicState waitState = FindTopicState::WAIT_TOPIC;
    int lastBotMsg = 0;
    int64_t userId = 0;
    int topMsgId = 0;
};

class FindByTopicCommand : public FindByFieldCommand<FindTopicSession> {
public:
    FindByTopicCommand(sqlite3* db, TgBot::Bot& bot, YandexDiskClient& yandex)
            : FindByFieldCommand<FindTopicSession>(db, bot, yandex, "topic",
                                                   "Введите тему/жанр книги (например, Фэнтези):") {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        auto& session = this->sessions[message->from->id];
        session.state = FindTopicSession::waitState;
        session.userId = message->from->id;

        auto topTopics = paginator.getTopTopics(10);
        if (!topTopics.empty()) {
            std::ostringstream oss;
            oss << u8"🔥 *ТОП-10 ТЕМ/ЖАНРОВ:*\n\n";
            int num = 1;
            for (const auto& topic : topTopics) {
                oss << num++ << ". " << topic << "\n";
            }
            session.topMsgId = bot.getApi().sendMessage(
                    message->chat->id,
                    oss.str(),
                    false,0,nullptr,"Markdown")->messageId;
        } else {
            session.topMsgId = 0;
        }
        session.lastBotMsg = bot.getApi().sendMessage(message->chat->id, prompt)->messageId;
    }
};

#endif // TG_BOT_FIND_BY_TOPICCOMMAND_H
