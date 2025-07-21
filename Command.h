#ifndef TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H
#define TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H


#include <tgbot/tgbot.h>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) = 0;
};
#endif //TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H
