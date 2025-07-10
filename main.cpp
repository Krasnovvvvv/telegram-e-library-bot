#include <tgbot/tgbot.h>
#include <cstdlib>
#include <iostream>
#include <string>

int main() {

    const char* token_cstr = std::getenv("BOT_TOKEN");
    if (token_cstr == nullptr) {
        std::cerr << "Error: BOT_TOKEN environment variable is not set." << std::endl;
        return 1;
    }
    std::string token(token_cstr);
    TgBot::Bot bot(token);

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Привет!");
    });

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        std::cout << "The user wrote: " << message->text << std::endl;
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Ваше сообщение: " + message->text);
    });

    try {
        std::cout << "Bot name: " << bot.getApi().getMe()->username << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
