#ifndef TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H
#define TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H

#pragma once

#include "ICommand.h"
#include "BookListPaginator.h"
#include "YandexDiskClient.h"

#include <sqlite3.h>

class CatalogCommand : public ICommand {
public:
    CatalogCommand(sqlite3* db_, TgBot::Bot& bot_, YandexDiskClient& yandex_)
            : db(db_), bot(bot_), yandex(yandex_), paginator(db_, bot_, yandex_) {}

    void execute(TgBot::Bot& /*bot*/, TgBot::Message::Ptr message) override {
        paginator.setUserPage(message->from->id, 0);
        paginator.sendPage(message->chat->id, message->from->id, "", {});
    }

private:
    sqlite3* db;
    TgBot::Bot& bot;
    YandexDiskClient& yandex;
    BookListPaginator paginator;
};

#endif // TG_BOT_ELECTRONIC_LIBRARY_CATALOGCOMMAND_H
