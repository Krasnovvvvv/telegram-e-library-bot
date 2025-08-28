# 📚 Telegram electronic book library bot

[![License](https://img.shields.io/github/license/Krasnovvvvv/telegram-e-library-bot)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/Krasnovvvvv/telegram-e-library-bot?style=social)](https://github.com/Krasnovvvvv/telegram-e-library-bot/stargazers)

Digital library Telegram bot: search, browse, and access books via chat. Built with C++ (tgbot-cpp) and SQLite

---

## ℹ️ About

`telegram-e-library-bot` is an open-source bot for Telegram that allows:

- **Search for books by title and author**
  
- **Quick catalog view**

- **Storing data in SQLite**

- **Intuitive chat interface**

- **Extensibility for learning tasks and new functions**

---

## 🗂️ Project Structure
```
telegram-e-library-bot/
├── include/                 # Public headers
├── src/                     # Source files (main.cpp)
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── LICENSE                  # License file
├── .gitignore               # Git ignore rules
```

---

## 🚀 Quick Start

### 🛠️ Prerequisites

- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.28 or newer
- TgBot — C++ library for Telegram Bot API
- ws2_32 — Windows sockets library (required on Windows only)
- unofficial-sqlite3 — CMake package for SQLite3
- fmt — Formatting library
- yandex-disk-cpp-client — Modern C++ static client for Yandex.Disk REST API  [![GitHub Repo](https://img.shields.io/badge/GitHub_Repo-222222?logo=github&logoColor=white&style=flat-square)](https://github.com/Krasnovvvvv/yandex-disk-cpp-client)
- libcurl — Required by both the Yandex.Disk client and for direct network operations
- Environment variable `YADISK_TOKEN` with your Yandex.Disk OAuth token **(full disk access)**
- Environment variable `BOT_TOKEN` with your telegram bot token **(get it from [`@BotFather`](https://t.me/BotFather))**

*Most of these dependencies (with the exception of system libraries such as ws2_32 in Windows) can be installed using your operating system's package manager or using fetchContent/CPM in CMake. I strongly recommend using the vcpkg package manager to simplify the installation of dependencies.*

**The Yandex.Disk client must be built or installed as a static library before creating this project!**


### 🏗️ Build Instructions

1. **Clone the Dependencies**

First, clone both the Yandex.Disk client and this project:
```sh
# Clone the Yandex.Disk C++ client library
git clone https://github.com/Krasnovvvvv/yandex-disk-cpp-client.git

# Clone this project (Telegram electronic library bot)
git clone https://github.com/Krasnovvvvv/telegram-e-library-bot.git

```
2. **Build the Yandex.Disk C++ client library**

```sh
cd yandex-disk-cpp-client
mkdir build && cd build
cmake ..
cmake --build .
cd ../..
```
3. **Build the Telegram electronic library bot**

```sh
cd telegram-e-library-bot
mkdir build && cd build
cmake .. \
  -DYANDEXDISK_INCLUDE_DIR=../../yandex-disk-cpp-client/include \
  -DYANDEXDISK_LIB_DIR=../../yandex-disk-cpp-client/build
cmake --build .
```
> **Note:**  
> Adjust the `YANDEXDISK_INCLUDE_DIR` and `YANDEXDISK_LIB_DIR` paths if your directory layout is different


### ⚙️ Personal settings

1. **Adding books**

In the file `main.cpp` you can find such a block of code:
```cpp
std::vector<BookInfo> books = {
            { "Гарри Поттер и философский камень", "Дж. К. Роулинг", "Фэнтези", "/files/harry_potter_1.pdf" },
            { "Гарри Поттер и Тайная комната", "Дж. К. Роулинг", "Фэнтези", "/files/harry_potter_2.pdf" }
    };
```
> Here you can add your books in the following format: `book title`, `author`, `book topic/genre`, `book path on your yandex.disk`

2. **The local path for downloading books from yandex.disk**

In the file `BookListPaginator.h` you can find such a block of code:
```cpp
try {
    std::filesystem::path dir("D:\\tmp");
    std::filesystem::create_directories(dir);
    std::filesystem::path localPath = dir / std::filesystem::path(path).filename();
    ...
}
```
> In the line `std::filesystem::path dir("D:\\tmp");` sets the local download path. Set your desired

---

## 🤖 Bot сommands overview

| Command             | Description                                                        |
|---------------------|--------------------------------------------------------------------|
| `/start`            | Start interacting with the bot                                     |
| `/catalog`          | Display the current book catalog                                   |
| `/find`             | Find a book by specifying both title and author (most precise)     |
| `/find_by_title`    | Find books by title, returns all books with the given title        |
| `/find_by_author`   | Find books by author, returns all books by the specified author    |
| `/find_by_topic`    | Find books by topic/genre, returns all books in that subject area  |

---

## 📦 Dependencies

| Dependency                  | Purpose                                               | Documentation / Repo                                         |
|-----------------------------|------------------------------------------------------|--------------------------------------------------------------|
| **TgBot**                   | C++ library to interact with the Telegram Bot API    | [GitHub](https://github.com/reo7sp/tgbot-cpp)                |
| **ws2_32**                  | Windows sockets library (networking, Windows only)   | [Microsoft Docs](https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2) |
| **unofficial-sqlite3**      | CMake package for embedding SQLite3                  | [vcpkg](https://github.com/microsoft/vcpkg/tree/master/ports/sqlite3) |
| **fmt**                     | Modern C++ formatting library                        | [GitHub](https://github.com/fmtlib/fmt)                      |
| **yandex-disk-cpp-client**  | Modern C++ client for Yandex.Disk REST API           | [GitHub](https://github.com/Krasnovvvvv/yandex-disk-cpp-client) |
| **libcurl**                 | Network requests (HTTP/HTTPS)                        | [GitHub](https://github.com/curl/curl) / [curl Docs](https://curl.se/libcurl/) |

> These dependecies are automatically handled via CMake (assuming installed on your system or via package managers like vcpkg)

---

## 🤝 Contribution

Contributions, bug reports, and feature requests are welcome!  
Please open issues or pull requests on the GitHub repository.

---

## 📝 License

This project is licensed under the MIT License — see [![License](https://img.shields.io/github/license/Krasnovvvvv/telegram-e-library-bot)](LICENSE).



