#ifndef TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H
#define TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H


#include <iostream>

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(const std::string& update) = 0;
};
#endif //TG_BOT_ELECTRONIC_LIBRARY_COMMAND_H
