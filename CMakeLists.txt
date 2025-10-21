cmake_minimum_required(VERSION 3.28)
project(tg_bot_electronic_library)

set(CMAKE_CXX_STANDARD 17)

find_package(tgbot-cpp CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(yandex-disk-cpp-client CONFIG REQUIRED)

add_executable(tg_bot_electronic_library src/main.cpp
        include/ICommand.h
        include/StartCommand.h
        include/CatalogCommand.h
        include/FindCommand.h
        include/FindByTitleCommand.h
        include/SessionCommand.h
        include/FindByAuthorCommand.h
        include/FindByTopicCommand.h
        include/FindByFieldCommand.h
        include/BookListPaginator.h)

target_link_libraries(tg_bot_electronic_library PRIVATE
        TgBot
        ws2_32
        unofficial::sqlite3::sqlite3
        fmt::fmt
        CURL::libcurl
        yandex-disk-cpp-client::yandex-disk-cpp-client
)