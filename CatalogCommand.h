#ifndef TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H
#define TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H


#include "ICommand.h"
#include <sqlite3.h>
#include <sstream>

class CatalogCommand : public ICommand {
private:
    sqlite3* db;
public:
    explicit CatalogCommand(sqlite3* db_) : db(db_) {}

    void execute(TgBot::Bot& bot, TgBot::Message::Ptr message) override {
        std::ostringstream catalogMsg;
        catalogMsg << "*Каталог книг:*\n\n";

        const char* sql = "SELECT title, author FROM books;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            bot.getApi().sendMessage(message->chat->id, "Ошибка при запросе к базе данных.");
            return;
        }

        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ++count;
            std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            std::string author(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            catalogMsg << count << ". _" << title << "_ — " << author << "\n";
        }
        sqlite3_finalize(stmt);

        if (count == 0) {
            catalogMsg.str(""); // очищаем
            catalogMsg << "*Каталог пуст \xF0\x9F\x98\xA2*";
        }

        bot.getApi().sendMessage(
                message->chat->id,
                catalogMsg.str(),
                false, 0, nullptr, "Markdown"
        );
    }
};

#endif //TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H
