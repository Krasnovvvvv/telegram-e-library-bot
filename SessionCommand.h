#ifndef TG_BOT_SESSIONCOMMAND_H
#define TG_BOT_SESSIONCOMMAND_H
#pragma once

#include "ICommand.h"
#include <unordered_map>

/**
 * Базовый класс для диалоговых команд с поддержкой сессий.
 * Хранит user_id -> SessionState и вызывает handleSessionMessage.
 */

template <typename Session>
class SessionCommand : public ICommand {
public:
    std::unordered_map<int64_t, Session> sessions;

    bool handleMessage(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        int64_t userId = message->from->id;
        auto it = sessions.find(userId);
        if (it == sessions.end())
            return false; // нет сессии — это не наша команда

        return handleSessionMessage(bot, message, it->second);
    }

protected:

    virtual bool handleSessionMessage(TgBot::Bot& bot,
                                      TgBot::Message::Ptr message,
                                      Session& session) = 0;
};

#endif // TG_BOT_SESSIONCOMMAND_H
