# Telegram electronic book library bot

[![License](https://img.shields.io/github/license/Krasnovvvvv/telegram-e-library-bot)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/Krasnovvvvv/telegram-e-library-bot?style=social)](https://github.com/Krasnovvvvv/telegram-e-library-bot/stargazers)

Digital library Telegram bot: search, browse, and access books via chat. Built with C++ (tgbot-cpp) and SQLite

---

## About

`telegram-e-library-bot` is an open-source bot for Telegram that allows:

- **Search for books by title and author**
  
- **Quick catalog view**

- **Storing data in SQLite**

- **Intuitive chat interface**

- **Extensibility for learning tasks and new functions**

---

## Project Structure
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

## Quick Start

### Prerequisites

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

### Build Instructions

1. **Clone the Dependencies**

First, clone both the Yandex.Disk client and this project:
```sh
# Clone the Yandex.Disk C++ client library
git clone https://github.com/Krasnovvvvv/yandex-disk-cpp-client.git

# Clone this project (Telegram electronic library bot)
git clone https://github.com/Krasnovvvvv/telegram-e-library-bot.git

```
2. **Build yandex-disk-cpp-client**
```sh
cd yandex-disk-cpp-client
mkdir build && cd build
cmake ..
cmake --build .
cd ../..
```




