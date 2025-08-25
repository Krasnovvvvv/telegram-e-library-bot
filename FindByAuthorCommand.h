#ifndef TG_BOT_FIND_BY_AUTHORCOMMAND_H
#define TG_BOT_FIND_BY_AUTHORCOMMAND_H

#pragma once

#include "FindByFieldCommand.h"

enum class FindAuthorState {
    NONE,
    WAIT_AUTHOR
};

struct FindAuthorSession {
    FindAuthorState state = FindAuthorState::NONE;
    static constexpr FindAuthorState waitState = FindAuthorState::WAIT_AUTHOR;
    int lastBotMsg = 0;
};

class FindByAuthorCommand : public FindByFieldCommand<FindAuthorSession> {
public:
    explicit FindByAuthorCommand(sqlite3* db)
            : FindByFieldCommand(db, "author", "Введите фамилию и инициалы автора книги (например, Дж. К. Роулинг):") {}
};

#endif // TG_BOT_FIND_BY_AUTHORCOMMAND_H
