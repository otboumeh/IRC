#include "Client.hpp"

Client::Client(int socketFd):_socket(socketFd),_nickName(""),_userName(""),_realName(""),\
_isAuthenticated(false),_isRegistered(false), _isVisible(true),_buffer(""){
};

Client::~Client(){
};
int Client::getSocket() const {
    return this->_socket;
}

const std::string& Client::getNickname() const {
    return this->_nickName;
}

const std::string& Client::getUsername() const {
    return this->_userName;
}
const std::string& Client::getRealname() const {
    return this->_realName;
}

bool Client::isAuthenticated() const {
    return this->_isAuthenticated;
}

bool Client::isRegistered() const {
    return this->_isRegistered;
}
void Client::appendBuffer(const std::string& data) {
    this->_buffer.append(data);
}

std::string& Client::getBuffer() {
    return this->_buffer;
}
void Client::setAuthenticated(bool auth) {
    this->_isAuthenticated = auth;
}
void Client::setNickname(const std::string& nick) {
    this->_nickName = nick;
}
void Client::setUsername(const std::string& user) {
    this->_userName = user;
}

void Client::setRealname(const std::string& real) {
    this->_realName = real;
}

void Client::setRegistered(bool reg) {
    this->_isRegistered = reg;
}