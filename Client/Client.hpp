#pragma once
#include <string> 
#include <iostream>

class Client{

	private:
	int 		_socket;
	std::string _nickName;
	std::string _userName;
	std::string _realName;

	bool 		_isAuthenticated;
	bool		_isRegistered;
	bool		_isVisible;
	std::string _buffer;

	public:

	Client() : _socket(-1) {} 
	Client(int socketFd);
	~Client();
	int getSocket() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
	const std::string& getRealname() const;
    bool isAuthenticated() const;
    bool isRegistered() const;
	void appendBuffer(const std::string& data);
	void setAuthenticated(bool auth);
	void setNickname(const std::string& nick);
	void setUsername(const std::string& user);
	void setRealname(const std::string& real);
	void setRegistered(bool reg);
    std::string& getBuffer();
	void setModoInvisible(bool estado) { _isVisible = estado; }
};