#include <tgbot/tgbot.h>
#include <iostream>

int main() {
    TgBot::Bot bot("ВАШ_ТОКЕН_БОТА");

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет!");
    });

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        std::cout << "Пользователь написал: " << message->text << std::endl;
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Ваше сообщение: " + message->text);
    });

    try {
        std::cout << "Имя бота: " << bot.getApi().getMe()->username << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
