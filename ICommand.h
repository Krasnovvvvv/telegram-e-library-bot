#ifndef TG_BOT_ICOMMAND_H
#define TG_BOT_ICOMMAND_H
#pragma once

#include <tgbot/tgbot.h>

/**
 * Базовый интерфейс для всех команд.
 * execute - вызов по /command
 * handleMessage - опциональная обработка обычных сообщений
 */

class ICommand {
public:
    virtual ~ICommand() = default;

    // Вызов при /команда
    virtual void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) = 0;

    // Обработка произвольных сообщений (default = ignore)
    virtual bool handleMessage(TgBot::Bot& bot, TgBot::Message::Ptr message) {
        return false;
    }
};

#endif // TG_BOT_ICOMMAND_H
