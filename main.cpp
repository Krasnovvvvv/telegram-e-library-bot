#include <tgbot/tgbot.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fmt/format.h>
#include <sqlite3.h>
#include <vector>
#include <map>
#include <memory>
#include "YandexDiskClient.h"
#include "StartCommand.h"
#include "CatalogCommand.h"

std::map<std::string, std::unique_ptr<ICommand>> commandRegistry;
sqlite3 *db;

struct BookInfo {
    std::string title;
    std::string author;
    std::string topic;
    std::string file_path;
};

bool add_books(sqlite3* db, const std::vector<BookInfo>& books) {
    const char* insert_sql =
            "INSERT OR IGNORE INTO books (title, author, topic, file_path) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    bool all_success = true;
    for (const auto& book : books) {
        sqlite3_bind_text(stmt, 1, book.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, book.author.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, book.topic.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, book.file_path.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Ошибка вставки: " << sqlite3_errmsg(db) << std::endl;
            all_success = false;
        } else {
            std::cout << "Книга \"" << book.title << "\" добавлена!" << std::endl;
        }

        sqlite3_reset(stmt); // сбрасываем stmt для следующей книги
    }
    sqlite3_finalize(stmt);
    return all_success;
}

void registerCommands() {
    commandRegistry["start"] = std::make_unique<StartCommand>();
    commandRegistry["catalog"] = std::make_unique<CatalogCommand>(db);
    // commandRegistry["help"] = std::make_unique<HelpCommand>();
    // ... other commands
}

void bindCommandHandlers(TgBot::Bot& bot) {
    for (auto& [name, cmd] : commandRegistry) {

        bot.getEvents().onCommand(
                name,
                [&bot, handler = cmd.get()](TgBot::Message::Ptr message) {
            handler->execute(bot, message);
        });
    }
}

int main() {

    // Block of database initialization
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

    std::vector<BookInfo> books = {
            { "Гарри Поттер и философский камень",
              "Дж. К. Роулинг", "Фэнтези",
              "/files/harry_potter_1.pdf" },
            { "Гарри Поттер и Тайная комната",
              "Дж. К. Роулинг",
              "Фэнтези",
              "/files/harry_potter_2.pdf" }
    };

    add_books(db, books);

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

    registerCommands();
    bindCommandHandlers(bot);

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (!message->text.empty() && message->text[0] == '/')
            return;
        bot.getApi().sendMessage(
                message->chat->id,
                u8"Кажется, я так ещё не умею. Пожалуйста, введите одну из доступных команд. "
                "Они доступны по кнопке *меню* слева снизу \xF0\x9F\x98\x89",
                false, 0, nullptr, "Markdown"
        );
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
