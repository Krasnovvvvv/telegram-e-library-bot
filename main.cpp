#include <tgbot/tgbot.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fmt/format.h>
#include <sqlite3.h>
#include <vector>
#include "YandexDiskClient.h"

int main() {

    // Block of database initialization
    sqlite3 *db;
    std::vector<const char*> sql_scripts;
    int rc = sqlite3_open("e_library_bot.db",&db);
    if(rc) {
        std::cerr<<fmt::format("Can't open database: {}", sqlite3_errmsg(db));
        return 1;
    }

    const char* create_users_table_sql = "CREATE TABLE IF NOT EXISTS users(tg_id INTEGER PRIMARY KEY,"
                                                                         " username TEXT);";

    const char* create_books_table_sql = "CREATE TABLE IF NOT EXISTS books(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                                                         " title TEXT NOT NULL,"
                                                                         " author TEXT NOT NULL,"
                                                                         " topic TEXT NOT NULL,"
                                                                         " file_path TEXT UNIQUE);";
    sql_scripts.push_back(create_users_table_sql);
    sql_scripts.push_back( create_books_table_sql);

    for(const auto &script:sql_scripts)
    {
        if(sqlite3_exec(db,script, nullptr, nullptr,nullptr) != SQLITE_OK) {
            std::cerr<<fmt::format("SQL error: {}", sqlite3_errmsg(db));
        }

    }

    sql_scripts.clear();

    const char* bot_token_cstr = std::getenv("BOT_TOKEN");
    if (bot_token_cstr == nullptr) {
        std::cerr << "Error: BOT_TOKEN environment variable is not set." << std::endl;
        return 1;
    }

    const char* disk_token_cstr = std::getenv("YADISK_TOKEN");
    if (!disk_token_cstr) {
        std::cerr << "Please set YADISK_TOKEN environment variable." << std::endl;
        return 1;
    }

    std::string bot_token(bot_token_cstr);
    std::string disk_token(disk_token_cstr);

    TgBot::Bot bot(bot_token);
    YandexDiskClient yandex(disk_token);

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
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
    });


    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        TgBot::User::Ptr user = message->from;
        std::cout << "The "<<user->firstName<<" wrote: " << message->text << std::endl;
        if (StringTools::startsWith(message->text, "/start") ||
            StringTools::startsWith(message->text, "/catalog") ||
            StringTools::startsWith(message->text, "/find") ||
            StringTools::startsWith(message->text, "/find_by_title") ||
            StringTools::startsWith(message->text, "/find_by_author") ||
            StringTools::startsWith(message->text, "/find_by_topic")) return;

        bot.getApi().sendMessage(message->chat->id, u8"Кажется, я так еще не умею. Пожалуйста, введите одну из доступных команд."
                                                    " Они доступны по кнопке *меню* слева снизу \xF0\x9F\x98\x89",
                                                    false,
                                                    0,
                                                    nullptr,
                                                    "Markdown");
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

    sqlite3_close(db);
    return 0;
}
