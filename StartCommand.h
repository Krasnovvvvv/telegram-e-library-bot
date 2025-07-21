#ifndef TG_BOT_ELECTRONIC_LIBRARY_STARTCOMMAND_H
#define TG_BOT_ELECTRONIC_LIBRARY_STARTCOMMAND_H
#pragma once

#include <fmt/core.h>
#include <tgbot/tgbot.h>
#include "ICommand.h"

class StartCommand : public ICommand {
public:
    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        bot.getApi().sendMessage(
                message->chat->id,
                fmt::format(u8"*Добро пожаловать в электронную библиотеку, {}! \xF0\x9F\x98\x8A*\n\n"
                            "Мои возможности:\n\n"
                            "*/start* - начать работу\n"
                            "*/catalog* - вывести текущий каталог книг\n"
                            "*/find* - найти книгу. Это наиболее точный поиск, указываешь "
                            "название книги и автора\n"
                            "*/find_by_title* - найти книгу по названию. Выдает список всех книг с таким названием\n"
                            "*/find_by_author* - найти книгу по автору. Выдает список всех книг этого автора\n"
                            "*/find_by_topic* - найти книгу по теме. Выдает список наиболее подходящих книг по этой теме\n\n"
                            "Все эти команды также доступны по кнопке *меню* слева снизу. Enjoy!",
                            message->from->firstName),
                false,
                0,
                nullptr,
                "Markdown"
        );
    }
};

#endif //TG_BOT_ELECTRONIC_LIBRARY_STARTCOMMAND_H
