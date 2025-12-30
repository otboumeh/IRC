#pragma once
#include <iostream>
#include <sys/socket.h>
#include <stdexcept>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <cstdio>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string>   
#include <sstream>
// #define FD_ZERO(fdsetp)
#include "../Client/Client.hpp"
#include "../Command/Command.hpp"
#include "../channel/channel.hpp"

class Channel;

class Server{
	private:
	int 		_port;
	std::string _password;
	int			_listeningSocketFd;	 // we give it valor -1 to set a safe default and return -1 in case of error
	//The listeningsocket is the doorman who will wait for the clients to arrive When a new client knocks, this doorman socket is the one we will use to call accept(). The accept() function is what creates the new, 
	//separate socket for the private conversation with that specific client.
	//it will never be used to send or receive actual chat msg .... only waiting new clients

	std::map<int , Client> _clients;
	std::vector<Channel> _Channels;

	fd_set _master_set;
    int    _max_fd;
	// I puted those two to make the server non copyable
	Server(const Server& other);
	Server&	operator=(const Server &other);
	
	//Method to setup my socket
	void setupSocket();
	void bindSocket();
	void startListening();
	void handleNewConnection();
	void handleClientData(int clientFd);
	void handleClientDisconnect(int clientFd);
    void processCommand(int clientFd, const std::string& command);
	void executeCommand(int clientFd, const Command& cmd);
	void handlePass(int clientFd, const Command& cmd);
    void handleNick(int clientFd, const Command& cmd);
    void handleUser(int clientFd, const Command& cmd);
	void handleWho(int clientFd, const Command& cmd);
	void reply(int clientFd, const std::string& message);
	public:
	//password by reference to not copy it and go exactly whre i have it
	Server(int port, const std::string &password);
	~Server();
	void run();

	// JOIN
	bool findChannelByName_b(std::vector<Channel>& channels, const std::string& name);
	Channel* findChannelByName(std::vector<Channel>& channels, const std::string& name);
	void handleJoin(int clientFd, const Command& cmd);
	void sendJoinMessages(Channel& ch, int clientFd);
	void new_join(std::string channel, int cl);

	// HANDLE CHANNEL
	int	search_fd_name(std::string name);
	void handleTopic(int clientFd, const Command& cmd);
	void handlePart(int clientFd, const Command& cmd);
	void handleKick(int clientFd, const Command& cmd);
	void handleInvite(int clientFd, const Command& cmd);
	void handleMode(int clientFd, const Command& cmd);
	void handleModeQuery(int clientFd, const Command& cmd);
	void handlePrivmsg(int clientFd, const Command& cmd);
	void handlePing(int clientFd, const Command& cmd);
	void handleQuit(int clientFd, const Command& cmd);
	ChannelError check_name(std::string name, int cl);
};

void sendReply(int clientFd, const std::string &msg);
