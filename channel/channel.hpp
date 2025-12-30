#ifndef CHANNELI_H
# define CHANNELI_H

# include <iostream>
# include <vector>
# include <algorithm>
# include <map>
#include "../Client/Client.hpp"

enum ChannelError {
    CHANNEL_OK = 0,
    ERR_NOT_ON_CHANNEL,
    ERR_BAD_CHANNEL_KEY,
    ERR_NOT_OPERATOR,
    ERR_USER_NOT_IN_CHANNEL,
	RPL_NO_TOPIC,
	ERR_NEED_MORE_PARAMS,
	ERR_CHANNEL_IS_FULL,
	ERR_INVITE_ONLY_CHAN,
	ERR_UNKNOWN_MODE,
	ERR_NO_SUCH_CHANNEL,
	ERR_USER_ON_CHANNEL,
	ERR_NO_SUCH_NICK,
	ERR_NO_RECIPIENT,
	ERR_NO_TEXT_TO_SEND,
	ERR_ERRONEUS_NICKNAME,
};

// Si los mensajes se envian desde client o donde sea, o en parseo, no hace falta esto, solo el error
struct ChannelEvent {
    ChannelError error;
    std::string notice; // Mensaje para enviar a todos (o vacío si no aplica)
};


class Channel
{
	private:
		std::string _name; // no more than 50 chars
		std::string _topic; // empty at the begining, no more than 307 chars.
		std::map<int, Client*> _clients;
		std::vector<int> _members;
		std::vector<int> _operators; // members wich are operators
		int _mode_flag[4]; // MODES (i, k, l, t) i?? 
		std::string _password; // Mode +k in the channel (NULL)
		std::vector<int> _invList; // invite list

		ChannelError change_mode_o(char flag, int other);
		ChannelError change_mode_l(char flag, std::string limit);
		ChannelError change_mode_k(char flag, std::string password);
		void change_mode_t(char flag);
		void change_mode_i(char flag);
	public:
		Channel();
		Channel(std::string name, int cl);
		~Channel();
		void send_privmsg(std::string cl); // PRIVMSG
		void send_notice(std::string cl); // NOTICE
		ChannelError change_mode(std::string mode, int client, int other_cl, std::string other);
		void print_channel_settings(void); // PARA HACER PRUEBAS

		ChannelError change_topic(int cl, std::string new_topic); // TOPIC
		ChannelError invite(int cl, int to_inv);
		ChannelError part(int cl, std::string msg);
		
		ChannelError kick(int cl, int other, std::string msg);
		
		//GETTERS
		std::string get_password();
		std::string get_name();
		std::vector<int> get_members() const;
		int *get_modes();
		std::vector<int> get_invite_list();
		std::string get_topic();

		// AUX TO JOIN_CHANNEL
		void add_member(int client, int flag);
		
		bool isOperator(int clientFd) const;
	};


#endif