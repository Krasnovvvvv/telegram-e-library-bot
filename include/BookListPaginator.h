#ifndef TG_BOT_BOOKLISTPAGINATOR_H
#define TG_BOT_BOOKLISTPAGINATOR_H

#pragma once

#include <sqlite3.h>
#include <vector>
#include <string>
#include <tgbot/tgbot.h>
#include <fmt/format.h>
#include <sstream>
#include <map>
#include <iostream>
#include <filesystem>
#include "YandexDiskClient.h"

struct BookItem {
    int id;
    std::string title;
    std::string author;
    std::string topic;
    std::string file_path;
};

class BookListPaginator {
public:
    explicit BookListPaginator(sqlite3* db_, TgBot::Bot& bot_, YandexDiskClient& yandex_)
            : db(db_), bot(bot_), yandex(yandex_) {}

    std::vector<BookItem> loadPage(const std::string& whereClause, const std::vector<std::string>& params, int page, int pageSize = 10) {
        std::vector<BookItem> books;
        std::ostringstream sql;
        sql << "SELECT rowid, title, author, topic, file_path FROM books ";
        if (!whereClause.empty()) sql << "WHERE " << whereClause << " ";
        sql << "ORDER BY rowid LIMIT ? OFFSET ?;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare paginator SQL: " << sqlite3_errmsg(db) << std::endl;
            return books;
        }

        int index = 1;
        for (const auto& param : params)
            sqlite3_bind_text(stmt, index++, param.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_bind_int(stmt, index++, pageSize);
        sqlite3_bind_int(stmt, index++, page * pageSize);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            BookItem item;
            item.id = sqlite3_column_int(stmt, 0);
            item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            item.author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            item.topic = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            item.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            books.push_back(item);
        }
        sqlite3_finalize(stmt);
        return books;
    }

    int loadTotalCount(const std::string& whereClause, const std::vector<std::string>& params) {
        std::ostringstream sql;
        sql << "SELECT COUNT(*) FROM books ";
        if (!whereClause.empty()) sql << "WHERE " << whereClause;

        sqlite3_stmt* stmt;
        int count = 0;
        if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare count SQL: " << sqlite3_errmsg(db) << std::endl;
            return 0;
        }

        int index = 1;
        for (const auto& param : params)
            sqlite3_bind_text(stmt, index++, param.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW)
            count = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);
        return count;
    }

    std::string formatMessage(const std::vector<BookItem>& books, int currentPage, int totalPages) {
        if(books.empty())
            return "*По вашему запросу книги не найдены* \xF0\x9F\x98\x94";

        std::ostringstream oss;
        oss << "*Список книг — страница " << (currentPage + 1) << "/" << totalPages << " :*\n\n";
        int num = currentPage * pageSize + 1;
        for(const auto &book: books)
            oss << num++ << ". *" << book.title << "* — _" << book.author << "_ (Тема: _" << book.topic << "_)\n";

        return oss.str();
    }

    TgBot::InlineKeyboardMarkup::Ptr buildKeyboard(const std::vector<BookItem>& books, int currentPage, int totalPages) {
        if(books.empty()) return nullptr; // Нет клавиатуры для пустого списка

        auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();

        for(auto &book : books) {
            auto btn = std::make_shared<TgBot::InlineKeyboardButton>();
            btn->text = "Скачать: " + book.title;
            btn->callbackData = "download_" + std::to_string(book.id);
            keyboard->inlineKeyboard.push_back({btn});
        }

        auto prev = std::make_shared<TgBot::InlineKeyboardButton>();
        prev->text = "⬅️";
        prev->callbackData = currentPage > 0 ? "page_" + std::to_string(currentPage - 1) : "ignore";

        auto info = std::make_shared<TgBot::InlineKeyboardButton>();
        info->text = std::to_string(currentPage + 1) + "/" + std::to_string(totalPages);
        info->callbackData = "ignore";

        auto next = std::make_shared<TgBot::InlineKeyboardButton>();
        next->text = "➡️";
        next->callbackData = currentPage + 1 < totalPages ? "page_" + std::to_string(currentPage + 1) : "ignore";
        keyboard->inlineKeyboard.push_back({prev, info, next});

        return keyboard;
    }

    void handleCallback(const TgBot::CallbackQuery::Ptr &callback,
                        const std::string &whereClause = "",
                        const std::vector<std::string> &params = {}) {
        try {
            std::string data = callback->data;
            int64_t chatId = callback->message->chat->id;
            int messageId = callback->message->messageId;

            if (data.rfind("page_", 0) == 0) {
                int page = std::stoi(data.substr(5));
                answerCallbackQuery(callback);
                setUserPage(callback->from->id, page);
                editPage(chatId, messageId, page, whereClause, params);
            } else if (data.rfind("download_", 0) == 0) {
                int bookId = std::stoi(data.substr(9));
                answerCallbackQuery(callback, "Загрузка книги...");

                // Удаляем сообщение с inline-клавиатурой
                try {
                    bot.getApi().deleteMessage(chatId, messageId);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to delete message: " << e.what() << std::endl;
                }

                sendBook(chatId, bookId);
            } else if (data == "ignore") {
                answerCallbackQuery(callback);
            }
        } catch (const TgBot::TgException &e) {
            std::cerr << "Callback query error: " << e.what() << std::endl;
        }
    }

    void answerCallbackQuery(const TgBot::CallbackQuery::Ptr &callback, const std::string &text = "") {
        try {
            bot.getApi().answerCallbackQuery(callback->id, text);
        } catch(const TgBot::TgException &e) {
            std::cerr << "Failed to answer callback query: " << e.what() << std::endl;
            // Пропускаем ошибку, т.к. запрос устарел
        }
    }

    void setUserPage(int64_t userId, int page) {
        userPages[userId] = page;
    }

    void sendPage(int64_t chatId, int64_t userId,
                  const std::string& whereClause,
                  const std::vector<std::string>& params) {
        int page = userPages.count(userId) ? userPages[userId] : 0;
        auto books = loadPage(whereClause, params, page, pageSize);
        int count = loadTotalCount(whereClause, params);
        int totalPages = (count + pageSize - 1) / pageSize;

        auto text = formatMessage(books, page, totalPages);

        if(books.empty())
            bot.getApi().sendMessage(chatId, text, false, 0, nullptr, "Markdown");
        else {
            auto keyboard = buildKeyboard(books, page, totalPages);
            bot.getApi().sendMessage(chatId, text, false, 0, keyboard, "Markdown");
        }
    }

    // Получить топ-10 популярных авторов
    std::vector<std::string> getTopAuthors(int limit = 10) {
        std::vector<std::string> authors;
        const char* sql = "SELECT author FROM author_requests ORDER BY request_count DESC LIMIT ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare top authors query: " << sqlite3_errmsg(db) << std::endl;
            return authors;
        }
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmt, 0);
            if (text)
                authors.emplace_back(reinterpret_cast<const char*>(text));
        }
        sqlite3_finalize(stmt);
        return authors;
    }

    // Получить топ-10 популярных тем
    std::vector<std::string> getTopTopics(int limit = 10) {
        std::vector<std::string> topics;
        const char* sql = "SELECT topic FROM topic_requests ORDER BY request_count DESC LIMIT ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare top topics query: " << sqlite3_errmsg(db) << std::endl;
            return topics;
        }
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmt, 0);
            if (text)
                topics.emplace_back(reinterpret_cast<const char*>(text));
        }
        sqlite3_finalize(stmt);
        return topics;
    }

    // Получить топ-10 популярных книг
    std::vector<std::pair<std::string, std::string>> getTopBooks(int limit = 10) {
        std::vector<std::pair<std::string, std::string>> books;
        const char* sql = "SELECT title, author FROM books ORDER BY request_count DESC LIMIT ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare top books query: " << sqlite3_errmsg(db) << std::endl;
            return books;
        }
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* t = sqlite3_column_text(stmt, 0);
            const unsigned char* a = sqlite3_column_text(stmt, 1);
            if (t && a)
                books.emplace_back(reinterpret_cast<const char*>(t), reinterpret_cast<const char*>(a));
        }
        sqlite3_finalize(stmt);
        return books;
    }

    // Увеличить счётчик запросов к автору
    void increaseAuthorRequestCount(const std::string& author) {
        const char* sql = "INSERT INTO author_requests (author, request_count) VALUES (?, 1) "
                          "ON CONFLICT(author) DO UPDATE SET request_count=request_count+1;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, author.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update author_requests: " << sqlite3_errmsg(db) << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    // Увеличить счётчик запросов к теме
    void increaseTopicRequestCount(const std::string& topic) {
        const char* sql = "INSERT INTO topic_requests (topic, request_count) VALUES (?, 1) "
                          "ON CONFLICT(topic) DO UPDATE SET request_count=request_count+1;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, topic.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update topic_requests: " << sqlite3_errmsg(db) << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    // Увеличить счётчик запросов конкретной книги по названию
    void increaseBookRequestCount(const std::string& title) {
        const char* sql = "UPDATE books SET request_count = request_count + 1 WHERE title = ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update book request count: " << sqlite3_errmsg(db) << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    std::vector<std::string> findMatchingAuthors(const std::string& userInput) {
        std::vector<std::string> authors;
        std::string pattern = "%" + userInput + "%";
        const char* sql = "SELECT DISTINCT author FROM books WHERE author LIKE ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* str = sqlite3_column_text(stmt, 0);
                if (str) authors.emplace_back(reinterpret_cast<const char*>(str));
            }
        }
        sqlite3_finalize(stmt);
        return authors;
    }

    std::vector<std::string> findMatchingTopics(const std::string& userInput) {
        std::vector<std::string> topics;
        std::string pattern = "%" + userInput + "%";
        const char* sql = "SELECT DISTINCT topic FROM books WHERE topic LIKE ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* str = sqlite3_column_text(stmt, 0);
                if (str) topics.emplace_back(reinterpret_cast<const char*>(str));
            }
        }
        sqlite3_finalize(stmt);
        return topics;
    }

    std::vector<std::pair<std::string, std::string>> findMatchingTitlesAuthors(const std::string& author, const std::string& title) {
        std::vector<std::pair<std::string, std::string>> books;
        std::string pattern1 = "%" + author + "%";
        std::string pattern2 = "%" + title + "%";
        const char* sql = "SELECT DISTINCT title, author FROM books WHERE author LIKE ? AND title LIKE ?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, pattern1.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, pattern2.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* titleStr = sqlite3_column_text(stmt, 0);
                const unsigned char* authorStr = sqlite3_column_text(stmt, 1);
                if (titleStr && authorStr)
                    books.emplace_back(reinterpret_cast<const char*>(titleStr), reinterpret_cast<const char*>(authorStr));
            }
        }
        sqlite3_finalize(stmt);
        return books;
    }

private:
    void editPage(int64_t chatId, int messageId, int page,
                  const std::string &whereClause,
                  const std::vector<std::string> &params) {
        auto books = loadPage(whereClause, params, page, pageSize);
        int count = loadTotalCount(whereClause, params);
        int totalPages = (count + pageSize - 1) / pageSize;

        auto text = formatMessage(books, page, totalPages);

        if(books.empty())
            bot.getApi().editMessageText(text, chatId, messageId, "", "Markdown", false, nullptr);
        else {
            auto keyboard = buildKeyboard(books, page, totalPages);
            bot.getApi().editMessageText(text, chatId, messageId, "", "Markdown", false, keyboard);
        }
    }

    void sendBook(int64_t chatId, int bookId) {
        const char* sql = "SELECT title, author, file_path FROM books WHERE rowid = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            try { bot.getApi().sendMessage(chatId, "Произошла ошибка при доступе к базе."); } catch(...) {}
            sqlite3_finalize(stmt);
            return;
        }

        sqlite3_bind_int(stmt, 1, bookId);

        std::string title;
        std::string author;
        std::string path;

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* textTitle = sqlite3_column_text(stmt, 0);
            if (textTitle)
                title = reinterpret_cast<const char*>(textTitle);

            const unsigned char* textAuthor = sqlite3_column_text(stmt, 1);
            if (textAuthor)
                author = reinterpret_cast<const char*>(textAuthor);

            const unsigned char* textPath = sqlite3_column_text(stmt, 2);
            if (textPath)
                path = reinterpret_cast<const char*>(textPath);
        }
        sqlite3_finalize(stmt);

        if (path.empty()) {
            try { bot.getApi().sendMessage(chatId, "Книга не найдена."); } catch(...) {}
            return;
        }

        try {
            std::filesystem::path dir("C:\\tmp");
            std::filesystem::create_directories(dir);

            if(isBiggerThan50MB(yandex.getResourceInfo(path)))
                throw "BigFile";

            std::filesystem::path localPath = dir / std::filesystem::path(path).filename();

            if (!std::filesystem::exists(localPath)) {

                bool ok = yandex.downloadFile(path, dir.string());
                if (!ok) {
                    try { bot.getApi().sendMessage(chatId, "Ошибка загрузки книги"); } catch(...) {}
                    std::cerr << "Ошибка загрузки книги \"" << title << "\" автора \"" << author << "\"" << std::endl;
                    return;
                }
            } else {
                std::cout << "Файл уже существует: " << localPath.string() << " (" << title << " — " << author << ")" << std::endl;
            }

            std::string ext = localPath.extension().string();
            std::string mimeType = "application/octet-stream";
            if (ext == ".pdf") mimeType = "application/pdf";
            else if (ext == ".epub") mimeType = "application/epub+zip";
            else if (ext == ".txt") mimeType = "text/plain";

            auto inputFile = TgBot::InputFile::fromFile(localPath.string(), mimeType);

            bot.getApi().sendDocument(chatId, inputFile);

        }

        catch(const char*) {

            yandex.publish(path);
            try { bot.getApi().sendMessage(chatId, fmt::format(u8"*Файл слиишком большой!* 😢"
                                             "\n\n Поэтому держи ссылку для скачивания: \n\n {}",
                                             yandex.getPublicDownloadLink(path)),
                                     false,
                                     0, nullptr, "Markdown");
            } catch (...) {}
        }

        catch (const std::exception& e) {

                try { bot.getApi().sendMessage(chatId, "Произошла ошибка во время отправки книги"); } catch (...) {}
                std::cerr << "Ошибка при загрузке/отправке книги \"" << title << "\" автора \"" << author << "\": "
                          << e.what() << std::endl;
        }
    }

    bool isBiggerThan50MB(const std::string& infoStr) {
        std::istringstream ss(infoStr);
        std::string line;
        while (std::getline(ss, line)) {

            auto pos = line.find("Size:");
            if (pos != std::string::npos) {
                double value = 0.0;
                std::string unit;
                std::istringstream sizeStream(line.substr(pos + 5));
                sizeStream >> value >> unit;
                return (value > 50.0)&&(unit == "MB");
            }
        }
    }

    sqlite3* db;
    TgBot::Bot& bot;
    YandexDiskClient& yandex;

    const static int pageSize = 10;
    std::map<int64_t, int> userPages;
};

#endif
