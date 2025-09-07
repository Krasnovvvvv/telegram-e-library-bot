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
#include <algorithm>

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

    // --- encoder: сериализует весь фильтр в callbackData ---
    std::string encodeCallback(const std::string& action, int page, const std::string& whereClause, const std::vector<std::string>& params) {
        std::string encoded = action + "_" + std::to_string(page) + "|" + whereClause + "|";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) encoded += "##";
            encoded += params[i];
        }
        return encoded;
    }

    // --- decoder: разбирает callbackData на action, page, whereClause, params ---
    bool decodeCallback(const std::string& data, std::string& action, int& page, std::string& whereClause, std::vector<std::string>& params) {
        size_t first_ = data.find('_');
        size_t firstBar = data.find('|');
        size_t secondBar = data.rfind('|');
        if (first_ == std::string::npos || firstBar == std::string::npos || secondBar == std::string::npos || secondBar <= firstBar)
            return false;
        action = data.substr(0, first_);
        page = std::stoi(data.substr(first_+1, firstBar-first_-1));
        whereClause = data.substr(firstBar+1, secondBar-firstBar-1);
        std::string paramsPart = data.substr(secondBar+1);
        params.clear();
        if (!paramsPart.empty())
        {
            size_t pos = 0;
            size_t next;
            while ((next = paramsPart.find("##", pos)) != std::string::npos) {
                params.push_back(paramsPart.substr(pos, next-pos));
                pos = next+2;
            }
            if (pos < paramsPart.length())
                params.push_back(paramsPart.substr(pos));
        }
        return true;
    }

    TgBot::InlineKeyboardMarkup::Ptr buildKeyboard(const std::vector<BookItem>& books, int currentPage, int totalPages,
                                                   const std::string& whereClause, const std::vector<std::string>& params) {
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
        prev->callbackData = currentPage > 0 ? encodeCallback("page", currentPage - 1, whereClause, params): "ignore";

        auto info = std::make_shared<TgBot::InlineKeyboardButton>();
        info->text = std::to_string(currentPage + 1) + "/" + std::to_string(totalPages);
        info->callbackData = "ignore";

        auto next = std::make_shared<TgBot::InlineKeyboardButton>();
        next->text = "➡️";
        next->callbackData = currentPage + 1 < totalPages ? encodeCallback("page", currentPage + 1, whereClause, params): "ignore";
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

            if (data.find("page_") == 0) {
                std::string action; int page = 0; std::string wc; std::vector<std::string> ps;
                if (!decodeCallback(data, action, page, wc, ps)) {
                    answerCallbackQuery(callback, "Ошибка данных пагинации");
                    return;
                }
                answerCallbackQuery(callback);
                setUserPage(callback->from->id, page);
                editPage(chatId, messageId, page, wc, ps);
            } else if (data.rfind("download_", 0) == 0) {
                int bookId = std::stoi(data.substr(9));
                answerCallbackQuery(callback, "Загрузка книги...");

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
            auto keyboard = buildKeyboard(books, page, totalPages, whereClause, params);
            bot.getApi().sendMessage(chatId, text, false, 0, keyboard, "Markdown");
        }
    }

    std::vector<std::string> getTopStrings(const char* sql, int limit) {
        std::vector<std::string> result;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare top query: " << sqlite3_errmsg(db) << std::endl;
            return result;
        }
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* v = sqlite3_column_text(stmt, 0);
            if (v) result.emplace_back(reinterpret_cast<const char*>(v));
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<std::pair<std::string, std::string>> getTopPairs(const char* sql, int limit) {
        std::vector<std::pair<std::string, std::string>> result;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare top query: " << sqlite3_errmsg(db) << std::endl;
            return result;
        }
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* v1 = sqlite3_column_text(stmt, 0);
            const unsigned char* v2 = sqlite3_column_text(stmt, 1);
            if (v1 && v2) result.emplace_back(
                        std::make_pair(
                                std::string(reinterpret_cast<const char*>(v1)),
                                std::string(reinterpret_cast<const char*>(v2))
                        )
                );
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<std::string> getTopAuthors(int limit = 10) {
        return getTopStrings("SELECT author FROM author_requests ORDER BY request_count DESC LIMIT ?;", limit);
    }
    std::vector<std::string> getTopTopics(int limit = 10) {
        return getTopStrings("SELECT topic FROM topic_requests ORDER BY request_count DESC LIMIT ?;", limit);
    }
    std::vector<std::pair<std::string, std::string>> getTopBooks(int limit = 10) {
        return getTopPairs("SELECT title, author FROM books ORDER BY request_count DESC LIMIT ?;", limit);
    }

    void increaseCount(const char* sql, const std::string& request) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, request.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update requests: " << sqlite3_errmsg(db) << std::endl;
            }
        }
        sqlite3_finalize(stmt);

    }

    void increaseAuthorRequestCount(const std::string& author) {
        increaseCount("INSERT INTO author_requests (author, request_count) VALUES (?, 1) "
                      "ON CONFLICT(author) DO UPDATE SET request_count=request_count+1;", author);
            }

    void increaseTopicRequestCount(const std::string& topic) {
        increaseCount("INSERT INTO topic_requests (topic, request_count) VALUES (?, 1) "
                      "ON CONFLICT(topic) DO UPDATE SET request_count=request_count+1;", topic);
    }

    void increaseBookRequestCount(const std::string& title) {
        increaseCount("UPDATE books SET request_count = request_count + 1 WHERE title = ?;", title);
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
            auto keyboard = buildKeyboard(books, page, totalPages, whereClause, params);
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
        return false;
    }

    sqlite3* db;
    TgBot::Bot& bot;
    YandexDiskClient& yandex;

    const static int pageSize = 10;
    std::map<int64_t, int> userPages;
};

#endif
