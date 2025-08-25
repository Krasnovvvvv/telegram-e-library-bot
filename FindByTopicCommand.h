#ifndef TG_BOT_FIND_BY_TOPICCOMMAND_H
#define TG_BOT_FIND_BY_TOPICCOMMAND_H

#pragma once

#include "FindByFieldCommand.h"

enum class FindTopicState {
    NONE,
    WAIT_TOPIC
};

struct FindTopicSession {
    FindTopicState state = FindTopicState::NONE;
    static constexpr FindTopicState waitState = FindTopicState::WAIT_TOPIC;
    int lastBotMsg = 0;
};

class FindByTopicCommand : public FindByFieldCommand<FindTopicSession> {
public:
    explicit FindByTopicCommand(sqlite3* db)
            : FindByFieldCommand(db, "topic", "Введите тему книги (например, Фэнтези):") {}
};

#endif // TG_BOT_FIND_BY_TOPICCOMMAND_H
