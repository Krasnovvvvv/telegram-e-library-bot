cmake_minimum_required(VERSION 3.28)
project(tg_bot_electronic_library)

set(CMAKE_CXX_STANDARD 17)

find_package(tgbot-cpp CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

set(YANDEXDISK_INCLUDE_DIR "" CACHE PATH "Path to YandexDiskClient headers")
set(YANDEXDISK_LIB_DIR "" CACHE PATH "Path to YandexDiskClient library directory")

add_executable(tg_bot_electronic_library main.cpp
        ICommand.h
        StartCommand.h
        CatalogCommand.h)

if(YANDEXDISK_INCLUDE_DIR)
    target_include_directories(tg_bot_electronic_library PRIVATE "${YANDEXDISK_INCLUDE_DIR}")
endif()
if(YANDEXDISK_LIB_DIR)
    target_link_directories(tg_bot_electronic_library PRIVATE "${YANDEXDISK_LIB_DIR}")
endif()

target_link_libraries(tg_bot_electronic_library PRIVATE
        TgBot
        ws2_32
        unofficial::sqlite3::sqlite3
        fmt::fmt
        yandex_disk_client
        CURL::libcurl
)


