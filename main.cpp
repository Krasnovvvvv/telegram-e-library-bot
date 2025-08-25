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
#include "ICommand.h"
#include "StartCommand.h"
#include "CatalogCommand.h"
#include "FindCommand.h"
#include "FindByTitleCommand.h"
#include "FindByAuthorCommand.h"
#include "FindByTopicCommand.h"
#include "BookListPaginator.h"

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
        std::cerr << "Request preparation error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    bool all_success = true;
    for (const auto& book : books) {
        sqlite3_bind_text(stmt, 1, book.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, book.author.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, book.topic.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, book.file_path.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Insertion error: " << sqlite3_errmsg(db) << std::endl;
            all_success = false;
        } else {
            std::cout << "The book \"" << book.title << "\" has been added!" << std::endl;
        }

        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    return all_success;
}

void registerCommands(TgBot::Bot& bot, YandexDiskClient& yandex) {
    commandRegistry["start"] = std::make_unique<StartCommand>();
    commandRegistry["catalog"] = std::make_unique<CatalogCommand>(db, bot, yandex);
    commandRegistry["find"] = std::make_unique<FindCommand>(db, bot, yandex);
    commandRegistry["find_by_title"] = std::make_unique<FindByTitleCommand>(db, bot, yandex);
    commandRegistry["find_by_author"] = std::make_unique<FindByAuthorCommand>(db, bot, yandex);
    commandRegistry["find_by_topic"] = std::make_unique<FindByTopicCommand>(db, bot, yandex);
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
    // инициализация базы
    int rc = sqlite3_open("e_library_bot.db", &db);
    if(rc) {
        std::cerr << fmt::format("Can't open database: {}", sqlite3_errmsg(db));
        return 1;
    }
    const char* create_users_table_sql = "CREATE TABLE IF NOT EXISTS users(tg_id INTEGER PRIMARY KEY, username TEXT);";
    const char* create_books_table_sql = "CREATE TABLE IF NOT EXISTS books(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                         " title TEXT NOT NULL, author TEXT NOT NULL, topic TEXT NOT NULL,"
                                         " file_path TEXT UNIQUE);";
    std::vector<const char*> sql_scripts = {create_users_table_sql, create_books_table_sql};

    for(const auto &script:sql_scripts) {
        if(sqlite3_exec(db, script, nullptr, nullptr,nullptr) != SQLITE_OK) {
            std::cerr << fmt::format("SQL error: {}", sqlite3_errmsg(db));
        }
    }

    std::vector<BookInfo> books = {
            { "Гарри Поттер и философский камень", "Дж. К. Роулинг", "Фэнтези", "/files/harry_potter_1.pdf" },
            { "Гарри Поттер и Тайная комната", "Дж. К. Роулинг", "Фэнтези", "/files/harry_potter_2.pdf" }
    };
    add_books(db, books);

    const char* bot_token_cstr = std::getenv("BOT_TOKEN");
    const char* disk_token_cstr = std::getenv("YADISK_TOKEN");
    if (!bot_token_cstr || !disk_token_cstr) {
        std::cerr << "Error: BOT_TOKEN or YADISK_TOKEN env variable not set" << std::endl;
        return 1;
    }

    TgBot::Bot bot(bot_token_cstr);
    YandexDiskClient yandex(disk_token_cstr);

    registerCommands(bot, yandex);
    bindCommandHandlers(bot);

    BookListPaginator paginator(db, bot, yandex);

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (!message->text.empty() && message->text[0] == '/')
            return;

        bool handled = false;
        for (auto& [name, cmd] : commandRegistry) {
            if (cmd->handleMessage(bot, message)) {
                handled = true;
                break;
            }
        }
        if (!handled) {
            bot.getApi().sendMessage(
                    message->chat->id,
                    u8"Кажется, я так ещё не умею. Воспользуйтесь *меню* 😉",
                    false, 0, nullptr, "Markdown"
            );
        }
    });

    bot.getEvents().onCallbackQuery([&](TgBot::CallbackQuery::Ptr query) {
        // Обработка callback (пагинация и скачивание)
        paginator.handleCallback(query);
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
