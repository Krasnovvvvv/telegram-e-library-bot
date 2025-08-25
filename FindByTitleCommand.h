#ifndef TG_BOT_FIND_BY_TITLECOMMAND_H
#define TG_BOT_FIND_BY_TITLECOMMAND_H

#pragma once

#include "FindByFieldCommand.h"

enum class FindTitleState {
    NONE,
    WAIT_TITLE
};

struct FindTitleSession {
    FindTitleState state = FindTitleState::NONE;
    static constexpr FindTitleState waitState = FindTitleState::WAIT_TITLE;
    int lastBotMsg = 0;
    int64_t userId = 0;
};

class FindByTitleCommand : public FindByFieldCommand<FindTitleSession> {
public:
    FindByTitleCommand(sqlite3* db, TgBot::Bot& bot, YandexDiskClient& yandex)
            : FindByFieldCommand(db, bot, yandex, "title",
                                 "Введите название книги (например, Занимательная физика):") {}
};

#endif // TG_BOT_FIND_BY_TITLECOMMAND_H
