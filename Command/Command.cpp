#include "Command.hpp"
#include <sstream>



Command::Command(const std::string& rawCommand) {
    std::string buffer = rawCommand;
    size_t      space_pos;

    // 1. Extract the command (the first word)
    space_pos = buffer.find(' ');
    if (space_pos != std::string::npos) {
        // If there's a space, the command is everything before it
        this->_command = buffer.substr(0, space_pos);
        // Erase the command and the space to prepare for parameter parsing
        buffer.erase(0, space_pos + 1);
    } else {
        // If there are no spaces, the whole string is the command
        this->_command = buffer;
        // No parameters, so we are done.
        return;
    }

    // 2. Parse the parameters
    while (!buffer.empty()) {
        // If the first character is a ':', it's the last parameter
        if (buffer[0] == ':') {
            this->_params.push_back(buffer.substr(1)); // Push everything after ':'
            break; // Stop parsing
        }

        // Find the next space
        space_pos = buffer.find(' ');
        if (space_pos != std::string::npos) {
            // If a space is found, the parameter is the word before it
            this->_params.push_back(buffer.substr(0, space_pos));
            // Erase the parameter and the space
            buffer.erase(0, space_pos + 1);
        } else {
            // If no more spaces, the rest of the buffer is the last parameter
            this->_params.push_back(buffer);
            break;
        }
    }
}

const std::string& Command::getCommand() const {
    return this->_command;
}

const std::vector<std::string>& Command::getParams() const {
    return this->_params;
}