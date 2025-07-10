cmake_minimum_required(VERSION 3.28)
project(tg_bot_electronic_library)

set(CMAKE_CXX_STANDARD 17)
find_package(tgbot-cpp CONFIG REQUIRED)

add_executable(tg_bot_electronic_library main.cpp)
target_link_libraries(tg_bot_electronic_library PRIVATE TgBot ws2_32)
