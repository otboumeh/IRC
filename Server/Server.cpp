#include "Server.hpp"
Server::Server(int port, const std::string& password) :
    _port(port),
    _password(password),
    _listeningSocketFd(-1)
{

    this->setupSocket();
    this->bindSocket();
    this->startListening();

    FD_ZERO(&this->_master_set);
    FD_SET(this->_listeningSocketFd, &this->_master_set);
    this->_max_fd = this->_listeningSocketFd;

    std::cout << "The server is running on port: " << _port << std::endl;
}


Server::~Server(){
	if (this->_listeningSocketFd != -1) {
        std::cout << "Closing listening socket fd: " << this->_listeningSocketFd << std::endl;
        close(this->_listeningSocketFd);
	}
};



//AF_INET: We're telling it we want to use the IPv4 protocol (e.g., 127.0.0.1).
//SOCK_STREAM: We're telling it we want a reliable TCP connection (the "phone call").
//0: We're letting the OS pick the specific protocol, which will be TCP.
void Server::setupSocket(){
	this->_listeningSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if(this->_listeningSocketFd == -1)
		throw std::runtime_error("Failed to create socket");
	int opt = 1; // This value means "enable the option"
    if (setsockopt(this->_listeningSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { // we use this funcion to disable the waiting time after the ctrl+c to use the same port and not wait 30 -60 sec
        throw std::runtime_error("Failed to set socket options");
    }
};




void Server::bindSocket(){
	sockaddr_in serverAddress;
	std::memset(&serverAddress,  0, sizeof(serverAddress) );
	serverAddress.sin_family = AF_INET;   //we store the address family
	serverAddress.sin_port = htons(_port);   //we store the port
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_listeningSocketFd,(sockaddr *)&serverAddress,sizeof(serverAddress)) < 0)
		throw std::runtime_error("Failed to bind socket"); 
};

void Server::startListening(){
	if (listen(_listeningSocketFd,SOMAXCONN)<0)
	{
		throw std::runtime_error("Failed to Listen"); 
	}
};
void Server::handleNewConnection() {
    sockaddr_in client_addr; // A structure to hold the new client's address
    socklen_t client_len = sizeof(client_addr);
    int new_socket_fd;

    // 1. Accept the new connection
    new_socket_fd = accept(this->_listeningSocketFd, (sockaddr *)&client_addr, &client_len);

    if (new_socket_fd < 0) {
        perror("accept() failed");
        return; 
    }

    // Optional: Print the new client's IP address maybe later we will delete it
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    std::cout << "New connection from " << client_ip << " on socket " << new_socket_fd << std::endl;

    // 2. Add the new socket to our master_set
    FD_SET(new_socket_fd, &this->_master_set);

    // 3. Update the maximum file descriptor
    if (new_socket_fd > this->_max_fd) {
        this->_max_fd = new_socket_fd;
    }

    // 4. Create a new Client object and add it to the map
    this->_clients.insert(std::make_pair(new_socket_fd, Client(new_socket_fd)));
}



void Server::handleClientDisconnect(int clientFd) {
    close(clientFd);

    FD_CLR(clientFd, &this->_master_set);

    this->_clients.erase(clientFd);

    std::cout << "Client " << clientFd << " has been disconnected and cleaned up." << std::endl;
}

void Server::processCommand(int clientFd, const std::string& rawCommand) {
    Command cmd(rawCommand);

    executeCommand(clientFd, cmd);
}

void Server::executeCommand(int clientFd, const Command& cmd) {
    const std::string& command = cmd.getCommand();

    if (command == "PASS") {
        handlePass(clientFd, cmd);
    } else if (command == "NICK") {
        handleNick(clientFd, cmd);
    } else if (command == "USER") {
        handleUser(clientFd, cmd);
    } else if (command == "JOIN") {
		handleJoin(clientFd, cmd);
	} else if (command == "TOPIC") {
		handleTopic(clientFd, cmd);
	} else if (command == "PART") {
		handlePart(clientFd, cmd);
	} else if (command == "KICK") {
		handleKick(clientFd, cmd);
	} else if (command == "INVITE") {
		handleInvite(clientFd, cmd);
	} else if (command == "MODE") {
		handleMode(clientFd, cmd);
	} else if (command == "PRIVMSG" || command == "NOTICE") {
	 	handlePrivmsg(clientFd, cmd);
	 }
	 else if (command == "WHO") {
		handleWho(clientFd, cmd);
	}
     else if (command == "QUIT") {
        handleQuit(clientFd, cmd);
    }
     else if (command == "PING") {
        handlePing(clientFd, cmd);
    }
    else {
        // Find the client who sent the command
        std::map<int, Client>::iterator it = this->_clients.find(clientFd);
        if (it != this->_clients.end()) {
            // Get the client's nickname (or a default if not set)
            std::string nick = it->second.getNickname().empty() ? "*" : it->second.getNickname();
            
            // Build the error message string
            std::string errorMsg = ":ircserv 421 " + nick + " " + command + " :Unknown command\r\n";
            
            // Send the message back to the client
            send(clientFd, errorMsg.c_str(), errorMsg.length(), 0);
        }
    }
}

void Server::handleClientData(int clientFd) {
    // Safety check, although the loop in run() should prevent this
    if (this->_clients.find(clientFd) == this->_clients.end()) return;
    
    Client& client = this->_clients.find(clientFd)->second;
    char    buffer[512];


    ssize_t bytes_received = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        handleClientDisconnect(clientFd);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    client.appendBuffer(std::string(buffer));


    // --- The processing loop ---
    std::string& clientBuffer = client.getBuffer();
    size_t pos;
    while ((pos = clientBuffer.find("\r\n")) != std::string::npos) {
        std::string command_line = clientBuffer.substr(0, pos);
        clientBuffer.erase(0, pos + 2);

        if (!command_line.empty()) {
            processCommand(clientFd, command_line);
        }
    }
}


void Server::run() {
    while (true) {
        fd_set working_set = this->_master_set;

        int activity = select(this->_max_fd + 1, &working_set, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select() failed");
            break;
        }


        for (int i = 0; i <= this->_max_fd; ++i) {
            if (FD_ISSET(i, &working_set)) {
                if (i == this->_listeningSocketFd) {
                    handleNewConnection();
                }
                else {
                    handleClientData(i);
                }
            }
        }
    }
}
void Server::handlePass(int clientFd, const Command& cmd) {
    // Find the client in the map
    std::map<int, Client>::iterator it = this->_clients.find(clientFd);
    if (it == this->_clients.end()) {
        return; // Safety check
    }
    Client& client = it->second;

    // Error: Client is already registered
    if (client.isRegistered()) {
        reply(clientFd, ":ircserv 462 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " :You may not reregister\r\n");
        return;
    }

    // Error: Client has already provided a password
    if (client.isAuthenticated()) {
        reply(clientFd, ":ircserv 462 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " :You may not reregister\r\n");
        return;
    }
    
    // Error: Must have exactly one parameter
    if (cmd.getParams().size() != 1) {
        reply(clientFd, ":ircserv 461 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " PASS :Not enough or too many parameters\r\n");
        return;
    }

    // Check the password
    if (cmd.getParams()[0] == this->_password) {
        // Correct password.
        client.setAuthenticated(true);
        std::cout << "Client " << clientFd << " authenticated successfully." << std::endl;
    } else {
        // Incorrect password
        reply(clientFd, ":ircserv 464 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " :Password incorrect\r\n");
    }
}

void Server::handleNick(int clientFd, const Command& cmd) {
    Client& client = this->_clients.find(clientFd)->second;

    // Check 1: Must be authenticated first
    if (!client.isAuthenticated()) {
        reply(clientFd, ":ircserv 451 " + client.getNickname() + " :You have not registered\r\n");
        return;
    }

    // Check 2: Must provide a nickname parameter
    if (cmd.getParams().size() != 1) {
        reply(clientFd, ":ircserv 431 " + client.getNickname() + " :No nickname given\r\n");
        return;
    }

    const std::string& newNick = cmd.getParams()[0];

    // Check 3: Basic nickname validation 
    if (newNick.empty() || newNick.length() > 9 || newNick.find_first_of(" ,*?!@.") != std::string::npos) {
        reply(clientFd, ":ircserv 432 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " " + newNick + " :Erroneous nickname\r\n");
        return;
    }

    // Check 4: Check if nickname is already in use
    for (std::map<int, Client>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it) {
        if (it->second.getNickname() == newNick) {
            reply(clientFd, ":ircserv 433 " + (client.getNickname().empty() ? "*" : client.getNickname()) + " " + newNick + " :Nickname is already in use\r\n");
            return;
        }
    }

    // If all checks pass, set the nickname
    std::cout << "Client " << clientFd << " changed nickname to " << newNick << std::endl;
    client.setNickname(newNick);
    // Note: We will add the logic to check for full registration and send welcome messages after USER is also implemented.
}

void Server::handleUser(int clientFd, const Command& cmd) {
    Client& client = this->_clients.find(clientFd)->second;

    // Check 1: Must be authenticated first
    if (!client.isAuthenticated()) {
        reply(clientFd, ":ircserv 451 " + client.getNickname() + " :You have not registered\r\n");
        return;
    }

    // Check 2: Don't allow reregistering
    if (client.isRegistered()) {
        reply(clientFd, ":ircserv 462 " + client.getNickname() + " :You may not reregister\r\n");
        return;
    }

    // Check 3: Must have 4 parameters
    if (cmd.getParams().size() != 4) {
        reply(clientFd, ":ircserv 461 " + client.getNickname() + " USER :Not enough parameters\r\n");
        return;
    }

    // Check 4: Must have a nickname set first
    if (client.getNickname().empty()) {
        reply(clientFd, ":ircserv 451 " + client.getNickname() + " :You have not registered\r\n");
        return;
    }

    // Action: Update client state
    client.setUsername(cmd.getParams()[0]);
    client.setRealname(cmd.getParams()[3]);
    client.setRegistered(true);

    // Action: Send Welcome Messages
    reply(clientFd, ":ircserv 001 " + client.getNickname() + " :Welcome to the Internet Relay Network " + client.getNickname() + "\r\n");
    reply(clientFd, ":ircserv 002 " + client.getNickname() + " :Your host is ircserv, running version 1.0\r\n");
    reply(clientFd, ":ircserv 003 " + client.getNickname() + " :This server was created some time ago\r\n");
    reply(clientFd, ":ircserv 004 " + client.getNickname() + " :ircserv 1.0 - -\r\n");
    reply(clientFd, ":ircserv 004 " + client.getUsername() + " this is username");
    reply(clientFd, ":ircserv 004 " + client.getRealname() + " this is realname\n");


    std::cout << "Client " << clientFd << " (" << client.getNickname() << ") is now fully registered." << std::endl;
}

void Server::reply(int clientFd, const std::string& message) {
    send(clientFd, message.c_str(), message.length(), 0);
}

void Server::handleJoin(int clientFd, const Command& cmd)
{
	if (cmd.getParams().size() < 1)
	{
		sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " JOIN :Not enough parameters\r\n");
		return ;
	}
	ChannelError err = check_name(cmd.getParams()[0], clientFd);
	if (err != CHANNEL_OK)
		return ;
	if (_Channels.empty() || findChannelByName_b(_Channels, cmd.getParams()[0]) == 0)
	{
		new_join(cmd.getParams()[0], clientFd);
		return ;
	}
	else
	{
		Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
		const std::vector<int>& members = ch->get_members();
		std::vector<int>::const_iterator it = std::find(members.begin(), members.end(), clientFd);
		if (it != members.end())
		{
			sendReply(clientFd, ":ircserv 443 " + _clients[clientFd].getNickname() + " " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :is already on channel\r\n");
			return ;
		}
		if (ch->get_modes()[1] == 0)
		{
			if (ch->get_modes()[2] < (int)ch->get_members().size())
			{
				// CHECK INVITATION MODE
				if (ch->get_modes()[0] == 0)
				{
					ch->add_member(clientFd, 1);
					sendJoinMessages(*ch, clientFd);
				}
				else
				{
					std::vector<int>::iterator it = std::find(ch->get_invite_list().begin(), ch->get_invite_list().end(), clientFd);
					if (it != ch->get_invite_list().end())
					{
						ch->add_member(clientFd, 1);
						sendJoinMessages(*ch, clientFd);
					}
					else
					{
						sendReply(clientFd, ":ircserv 473 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+i)\r\n");
						return ;
					}
				}
			}
			else
			{
				sendReply(clientFd, ":ircserv 471 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+l)\r\n");
				return ;
			}
		}
		else
		{
			if (ch->get_modes()[1] == 1 && cmd.getParams().size() < 2)
			{
				sendReply(clientFd, ":ircserv 475 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+k)\r\n");
				return ;
			}
			// YOU need a password
			if (cmd.getParams()[1] == ch->get_password() && ch->get_modes()[1] == 1)
			{
				// check limit 
				if (ch->get_modes()[2] < (int)ch->get_members().size())
				{
					// CHECK INVITATION MODE
					if (ch->get_modes()[0] == 0)
					{
						ch->add_member(clientFd, 1);
						sendJoinMessages(*ch, clientFd);
					}
					else
					{
						std::vector<int>::iterator it = std::find(ch->get_invite_list().begin(), ch->get_invite_list().end(), clientFd);
						if (it != ch->get_invite_list().end())
						{
							ch->add_member(clientFd, 1);
							sendJoinMessages(*ch, clientFd);
						}
						else
						{
							sendReply(clientFd, ":ircserv 473 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+i)\r\n");
							return ;
						}
					}
				}
				else
				{
					sendReply(clientFd, ":ircserv 471 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+l)\r\n");
					return ;
				}
			}
			else	
			{
				sendReply(clientFd, ":ircserv 475 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+k)\r\n");
				return ;
			}
		}
	}
}

Channel* Server::findChannelByName(std::vector<Channel>& channels,const std::string& name)
{
    for (std::vector<Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
        if (it->get_name() == name)
			return &(*it);
    }
	return NULL;
}

bool Server::findChannelByName_b(std::vector<Channel>& channels,const std::string& name)
{
    for (std::vector<Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
        if (it->get_name() == name)
			return 1;
    }
    return 0;
}

void Server::sendJoinMessages(Channel& ch, int clientFd)
{
    std::string nick;
    std::string user;
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it != _clients.end())
	{
        nick = it->second.getNickname();
        user = it->second.getUsername();
    }
    // Mensaje JOIN a todos los miembros (incluido el nuevo)
    std::string joinMsg = ":" + nick + "!" + user + "@localhost JOIN :" + ch.get_name() + "\r\n";
    const std::vector<int>& members = ch.get_members();
    for (size_t i = 0; i < members.size(); ++i)
	{
        sendReply(members[i], joinMsg);
    }
    // Enviar topic actual o "No topic set" al cliente que entra
    if (ch.get_topic().empty()) {
        sendReply(clientFd, ":ircserv 331 " + nick + " " + ch.get_name() + " :No topic is set\r\n");
    } else {
        sendReply(clientFd, ":ircserv 332 " + nick + " " + ch.get_name() + " :" + ch.get_topic() + "\r\n");
    }
    // Envio a todos los miembtros
    std::string nameList;
    for (size_t i = 0; i < members.size(); ++i)
	{
        int memberFd = members[i];
        std::string prefix = "";
        if (ch.isOperator(memberFd))
            prefix = "@";
        nameList += prefix + _clients[memberFd].getNickname() + " ";
    }
    if (!nameList.empty())
        nameList.erase(nameList.size() - 1);
    sendReply(clientFd, ":ircserv 353 " + nick + " = " + ch.get_name() + " :" + nameList + "\r\n");
    sendReply(clientFd, ":ircserv 366 " + nick + " " + ch.get_name() + " :End of /NAMES list.\r\n");
}

void Server::new_join(std::string channel, int cl)
{
		Channel ch(channel, cl);
		_Channels.push_back(ch);
		sendJoinMessages(ch, cl);
}


void Server::handleTopic(int clientFd, const Command& cmd)
{
    std::string serverName = "ircserv";
    std::string nick = _clients[clientFd].getNickname();
    std::string user = _clients[clientFd].getUsername();
    std::string host = "localhost";
    if (cmd.getParams().empty())
	{
        sendReply(clientFd, ":" + serverName + " 461 " + nick + " TOPIC :Not enough parameters\r\n");
        return ;
    }
    Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
    if (ch == NULL)
	{
        sendReply(clientFd, ":" + serverName + " 403 " + nick + " " + cmd.getParams()[0] + " :No such channel\r\n");
        return ;
    }
    if (cmd.getParams().size() < 2)
	{
        if (ch->get_topic().empty())
		{
            sendReply(clientFd, ":" + serverName + " 331 " + nick + " " + ch->get_name() + " :No topic is set\r\n");
        } else {
            sendReply(clientFd, ":" + serverName + " 332 " + nick + " " + ch->get_name() + " :" + ch->get_topic() + "\r\n");
        }
        return ;
    }
	std::string new_topic = "";
    if (cmd.getParams().size() > 1)
		new_topic = cmd.getParams()[1];
    ChannelError err = ch->change_topic(clientFd, new_topic);
	if (err != CHANNEL_OK)
	{
		if (err == ERR_NOT_ON_CHANNEL)
			sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " : that channel\r\n");
		else if (err == ERR_NOT_OPERATOR)
			sendReply(clientFd, ":ircserv 482 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not channel operator\r\n");
		else if (err == RPL_NO_TOPIC)
			sendReply(clientFd, ":ircserv 331 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :No topic is set\r\n");
		return ;
	}
    std::string topicMsg = ":" + nick + "!" + user + "@" + host + " TOPIC " + ch->get_name() + " " + new_topic + "\r\n";
	// envio a todos los del canal
    const std::vector<int>& members = ch->get_members();
	for (size_t i = 0; i < members.size(); ++i)
		sendReply(members[i], topicMsg);
	return ;
}


void Server::handlePart(int clientFd, const Command& cmd)
{
	if (cmd.getParams().size() == 0)
	{
		sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " PART :Not enough parameters\r\n");
		return ;
	}
	Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
	if (ch == NULL)
	{
		sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such channel\r\n");
        return ;
	}
    std::string reason = "";
    if (cmd.getParams().size() > 1)
        reason = cmd.getParams()[1];
    std::string nick = _clients[clientFd].getNickname();
    std::string user = _clients[clientFd].getUsername();
    std::string host = "localhost";
    std::string partMsg = ":" + nick + "!" + user + "@" + host + " PART " + ch->get_name() + " " + reason + "\r\n";
	// Envio a todos los del canal
    const std::vector<int>& members = ch->get_members();
    for (size_t i = 0; i < members.size(); ++i)
		sendReply(members[i], partMsg);
	ChannelError err = ch->part(clientFd, reason);
	if (err == ERR_USER_NOT_IN_CHANNEL)
		sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not on that channel\r\n");
}

void Server::handleKick(int clientFd, const Command& cmd)
{
	if (cmd.getParams().size() < 2)
	{
		sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " KICK :Not enough parameters\r\n");
		return ;
	}
	Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
	if (ch == NULL)
	{
		sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such channel\r\n");
        return ;
	}
	const std::vector<int>& memberss = ch->get_members();
	std::vector<int>::const_iterator ite = std::find(memberss.begin(), memberss.end(), search_fd_name(cmd.getParams()[1]));
	if (ite == memberss.end())
	{
		// ERROR -> NO ESTA EL OTRO EN EL CANAL
		sendReply(clientFd, ":ircserv 441 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[1] + " " + ch->get_name() + " :They aren't on that channel\r\n");
		return ;
	}
    std::string reason = "";
	if (cmd.getParams().size() == 3)
		reason = cmd.getParams()[2];
    std::string nick, user, host = "localhost";
    std::map<int, Client>::iterator it = _clients.find(clientFd);
    if (it != _clients.end())
    {
        nick = it->second.getNickname();
        user = it->second.getUsername();
    }
    else
    {
        nick = "*";
        user = "*";
    }
    std::string kickMsg = ":" + nick + "!" + user + "@" + host + " KICK " + ch->get_name() + " " + _clients[search_fd_name(cmd.getParams()[1])].getNickname();
    if (!reason.empty())
	{
		kickMsg += " :";
		kickMsg += reason;
	}
    kickMsg += "\r\n";
	// Envio a todos los del canal
	ChannelError err = ch->kick(clientFd, search_fd_name(cmd.getParams()[1]), reason);
	if (err == ERR_NOT_OPERATOR)
	{
		sendReply(clientFd, ":ircserv 482 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not channel operator\r\n");
		return ;
	}
    const std::vector<int>& members = ch->get_members();
	for (size_t i = 0; i < members.size(); ++i)
		sendReply(members[i], kickMsg);
	sendReply(search_fd_name(cmd.getParams()[1]), kickMsg);
}

void Server::handleInvite(int clientFd, const Command& cmd)
{
	if (cmd.getParams().size() < 2)
	{
		sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " INVITE :Not enough parameters\r\n");
		return ;
	}
	Channel* ch = findChannelByName(_Channels, cmd.getParams()[1]);
	if (ch == NULL)
	{
		sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[1] + " :No such channel\r\n");
        return ;
	}
	if (search_fd_name(cmd.getParams()[0]) == 0)
	{
		sendReply(clientFd, ":ircserv 441 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " " + ch->get_name() + " :They aren't on that channel\r\n");
		return ;
	}
	ChannelError err = ch->invite(clientFd, search_fd_name(cmd.getParams()[0]));
	if (err != CHANNEL_OK)
	{
		if (err == ERR_NOT_ON_CHANNEL)
			sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not on that channel\r\n");
		else if (err == ERR_NOT_OPERATOR)
			sendReply(clientFd, ":ircserv 482 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not channel operator\r\n");
		return ;
	}
    std::string inviterNick = _clients[clientFd].getNickname();
    std::string targetNick = _clients[search_fd_name(cmd.getParams()[0])].getNickname();
    std::string channelName = ch->get_name();
    // Enviar mensaje al nick invitado
    std::string inviteMsg = ":" + inviterNick + "!" + _clients[clientFd].getUsername() + "@localhost INVITE " + targetNick + " :" + channelName + "\r\n";
    sendReply(search_fd_name(cmd.getParams()[0]), inviteMsg);
    // Enviar mensaje de confirmacion al que manda el mensaje
    std::string confirmMsg = ":ircserv 341 " + inviterNick + " " + targetNick + " " + channelName + "\r\n";
    sendReply(clientFd, confirmMsg);
}
void Server::handleQuit(int clientFd, const Command& cmd) {
    (void)cmd;
    std::cout << "Client " << clientFd << " sent QUIT command. Disconnecting." << std::endl;
    
    handleClientDisconnect(clientFd);
}

void Server::handleModeQuery(int clientFd, const Command& cmd)
{
	Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
	if (ch == NULL)
	{
		sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such channel\r\n");
        return ;
	}
	const std::vector<int>& members = ch->get_members();
	std::vector<int>::const_iterator it = std::find(members.begin(), members.end(), clientFd);
	if (it == members.end())
	{
		// ERROR -> NO ESTAS EN EL CANAL
		sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :You're not on that channel\r\n");
		return ;
	}
	std::string senderNick = _clients[clientFd].getNickname();
	std::string user = _clients[clientFd].getUsername();
	std::string channelName = ch->get_name();
	std::string modes = "+";
	std::string params = "";
	if (ch->get_modes()[0] == 1) modes += "i";
	if (ch->get_modes()[3] == 1) modes += "t";
	if (ch->get_modes()[1] == 1) 
	{
		modes += "k";
		params += " " + ch->get_password();
	}
	if (ch->get_modes()[2] != -1) 
	{
		modes += "l";
		std::stringstream ss;
		ss << ch->get_modes()[2];
		params += " " + ss.str();
	}
	std::string modeMsg = ":ircserv 324 " + senderNick + " " + channelName + " " + modes + params + "\r\n";
	sendReply(clientFd, modeMsg);
}

void Server::handleMode(int clientFd, const Command& cmd)
{
	if (cmd.getParams().size() == 1)
		return handleModeQuery(clientFd, cmd);
    else if (cmd.getParams().size() < 2)
    {
        sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " MODE :Not enough parameters\r\n");
		return ;
    }
    // Modo usuario -> mode USER +i
    if (cmd.getParams()[1] == "+i" || cmd.getParams()[1] == "-i")
    {
        Channel* chc = findChannelByName(_Channels, cmd.getParams()[0]);
        if (chc == NULL)
        {
            int cc = search_fd_name(cmd.getParams()[0]);
            if (cc != 0)
            {
                std::map<int, Client>::iterator ite = _clients.find(clientFd);
                if (ite != _clients.end())
                {
                    // Aquí asumo que +i significa modo invisible para usuario
                    bool modoInvisible = (cmd.getParams()[1] == "+i");
                    ite->second.setModoInvisible(modoInvisible);
                    return ;
                }
            }
            else
            {
                sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :You're not on that channel\r\n");
				return ;
            }
        }
    }
    Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
    if (ch == NULL)
    {
        sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such channel\r\n");
        return ;
    }
    ChannelError err;
	std::string target;
    // Dependiendo del modo, parametros distintos
    if (cmd.getParams()[1] == "+o" || cmd.getParams()[1] == "-o")
    {
        if (cmd.getParams().size() < 3)
		{
            sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " MODE :Not enough parameters\r\n");
			return ;
        }
		target = cmd.getParams()[2];
        err = ch->change_mode(cmd.getParams()[1], clientFd, search_fd_name(cmd.getParams()[2]), "");
    }
    else if (cmd.getParams()[1] == "+l" || cmd.getParams()[1] == "+k")
    {
        if (cmd.getParams().size() < 3)
		{
			sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " MODE :Not enough parameters\r\n");
			return ;
        }
		target = cmd.getParams()[2];
        err = ch->change_mode(cmd.getParams()[1], clientFd, 0, cmd.getParams()[2]);
    }
    else
		err = ch->change_mode(cmd.getParams()[1], clientFd, 0, "");
	if (err != CHANNEL_OK)
	{
		if (err == ERR_UNKNOWN_MODE)
			sendReply(clientFd, ":ircserv 472 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[1] + " :is unknown mode char to me\r\n");
		else if (err == ERR_NEED_MORE_PARAMS)
			sendReply(clientFd, ":ircserv 461 " + _clients[clientFd].getNickname() + " MODE :Not enough parameters\r\n");
		else if (err == ERR_BAD_CHANNEL_KEY)
			sendReply(clientFd, ":ircserv 475 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :Cannot join channel (+k)\r\n");
		else if (err == ERR_NOT_ON_CHANNEL)
			sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not on that channel\r\n");
		else if (err == ERR_NOT_OPERATOR)
			sendReply(clientFd, ":ircserv 482 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not channel operator\r\n");
		else if (err == ERR_USER_NOT_IN_CHANNEL)
			sendReply(clientFd, ":ircserv 441 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[2] + " " + ch->get_name() + " :They aren't on that channel\r\n");
		return ;
	}
    // Mensaje a todos los usuarios
	std::string senderNick = _clients[clientFd].getNickname();
	std::string user = _clients[clientFd].getUsername();
	std::string channelName = ch->get_name();
	std::string host = "localhost";  // O tu host real
	std::string modeMsg = ":" + senderNick + "!" + user + "@" + host + " MODE " + channelName + " " + cmd.getParams()[1];
	if (!target.empty())
		modeMsg += " " + target;
	modeMsg += "\r\n";
    const std::vector<int>& members = ch->get_members();
    for (size_t i = 0; i < members.size(); ++i)
        sendReply(members[i], modeMsg);
}

void Server::handlePing(int clientFd, const Command& cmd) {
    std::string token = "ircserv"; 

    if (!cmd.getParams().empty()) {
        token = cmd.getParams()[0];
    }

    std::string pong_reply = ":ircserv PONG ircserv :" + token + "\r\n";
    
    reply(clientFd, pong_reply);
}


void Server::handlePrivmsg(int clientFd, const Command& cmd)
 {
	int flag = 0;

 	if (cmd.getParams().size() < 1)
 	{
		std::string cmd_msg;
		if (cmd.getCommand() == "PRIVMSG")
			cmd_msg = "PRIVMSG";
		else
			cmd_msg = "NOTICE";
		sendReply(clientFd, ":ircserv 411 " + _clients[clientFd].getNickname() + " :No recipient given " + cmd_msg + "\r\n");
 		return ;
 	}
 	else if (cmd.getParams().size() < 2)
 	{
		sendReply(clientFd, ":ircserv 412 " + _clients[clientFd].getNickname() + " :No text to send\r\n");
 		return ;
 	}
 	Channel* ch = findChannelByName(_Channels, cmd.getParams()[0]);
 	if (ch == NULL)
 	{
 		if (cmd.getParams()[0].empty() || cmd.getParams()[0].length() > 9 || cmd.getParams()[0].find_first_of(" ,*?!@.") != std::string::npos)
 		{
			sendReply(clientFd, ":ircserv 432 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :Erroneous nickname\r\n");
 			return ;
 		}
		else if (search_fd_name(cmd.getParams()[0]) == 0)
		{
			sendReply(clientFd, ":ircserv 401 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such nick/channel\r\n");
			return ;
		}
		else
			flag = 1;
		if (flag == 0)
		{
			sendReply(clientFd, ":ircserv 403 " + _clients[clientFd].getNickname() + " " + cmd.getParams()[0] + " :No such channel\r\n");
        	return ;
		}
 	}

 	std::string senderNick = _clients[clientFd].getNickname();
 	std::string senderUser = _clients[clientFd].getUsername();
 	std::string senderHost = "localhost";
 	std::string target = cmd.getParams()[0];
 	std::string message = cmd.getParams()[1];
	 std::string fullMsg;
	if (cmd.getCommand() == "PRIVMSG")
 		fullMsg = ":" + senderNick + "!" + senderUser + "@" + senderHost + " PRIVMSG " + target + " :" + message + "\r\n";
	else
		fullMsg = ":" + senderNick + "!" + senderUser + "@" + senderHost + " NOTICE " + target + " :" + message + "\r\n";
 	if (flag == 0)
 	{
		const std::vector<int>& members = ch->get_members();
		std::vector<int>::const_iterator it = std::find(members.begin(), members.end(), clientFd);
		if (it == members.end())
		{
			sendReply(clientFd, ":ircserv 442 " + _clients[clientFd].getNickname() + " " + ch->get_name() + " :You're not on that channel\r\n");
			return ;
		}
 		for (std::vector<int>::const_iterator it = members.begin(); it != members.end(); ++it)
 		{
 			int memberFd = *it;
 			if (memberFd != clientFd)
 				sendReply(memberFd, fullMsg);
 		}
 				return ;
 	}
	// Si es user
 	sendReply(search_fd_name(cmd.getParams()[0]), fullMsg);
}


int	Server::search_fd_name(std::string name)
{
	for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.getNickname() == name)
			return it->first;
	}
	return 0;
}

void sendReply(int clientFd, const std::string &msg)
{
    // Enviar el mensaje al cliente
    ssize_t bytesSent = send(clientFd, msg.c_str(), msg.length(), 0);

    if (bytesSent == -1) {
        perror("send");
        std::cerr << "Error enviando mensaje al cliente FD: " << clientFd << std::endl;
    }
    else if ((size_t)bytesSent < msg.length()) {
        std::cerr << "Advertencia: no se enviaron todos los bytes al cliente FD: " << clientFd << std::endl;
    }
}

ChannelError Server::check_name(std::string name, int cl)
{
	if (name.length() > 50)
	{
		sendReply(cl, ":ircserv 403 " + _clients[cl].getNickname() + " " + name + " :No such channel\r\n");
        return ERR_NO_SUCH_CHANNEL;
	}
	else if (name[0] != '#')
	{
		sendReply(cl, ":ircserv 403 " + _clients[cl].getNickname() + " " + name + " :No such channel\r\n");
        return ERR_NO_SUCH_CHANNEL;
	}
	for(unsigned int i = 0; i < name.length(); i++)
	{
		if (name[i] == ' ' || name[i] == ',' || name[i] == '\x07')
		{
			sendReply(cl, ":ircserv 403 " + _clients[cl].getNickname() + " " + name + " :No such channel\r\n");
        	return ERR_NO_SUCH_CHANNEL;
		}
	}
	return CHANNEL_OK;
}

void Server::handleWho(int clientFd, const Command& cmd) {
    if (cmd.getParams().empty()) {
        return;
    }
    Client& client = this->_clients.find(clientFd)->second;
    const std::string& target = cmd.getParams()[0];
    Channel* channel = findChannelByName(_Channels, target);
    if (channel == NULL) {
        reply(clientFd, ":ircserv 315 " + client.getNickname() + " " + target + " :End of /WHO list.\r\n");
        return;
    }
    const std::vector<int>& members = channel->get_members();

    for (size_t i = 0; i < members.size(); ++i) {
        int memberFd = members[i];
        Client& member = this->_clients.find(memberFd)->second;
        
        std::string status = "";
        if (channel->isOperator(memberFd)) {
            status = "@";
        }
        std::string replyMsg = ":ircserv 352 " + client.getNickname() + " " + channel->get_name()
                             + " " + member.getUsername() + " localhost ircserv " + member.getNickname()
                             + " H" + status + " :0 " + member.getRealname() + "\r\n";
        
        reply(clientFd, replyMsg);
    }
    reply(clientFd, ":ircserv 315 " + client.getNickname() + " " + target + " :End of /WHO list.\r\n");
}
