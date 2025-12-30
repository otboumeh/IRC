#include "channel.hpp"

Channel::Channel(): _name("")
{
	return ; 
}

Channel::Channel(std::string name, int cl)
{
	_name = name;
	_operators.push_back(cl); // meto el usuario actual
	_members.push_back(cl); // meto el usuario actual
	// Modos desactivados, orden alfabetico, excepto +o
	_mode_flag[0] = 0; // +i
	_mode_flag[1] = 0; // +k
	_mode_flag[2] = -1; // +l
	_mode_flag[3] = 0; // +t

}

Channel::~Channel() {}

// NOMBRE DEL CANAL CORRECTO O NO (0 -> OK, 1-> OUT)
// 403 nick #canal_invalido :No such channel -> 2812 IRC

ChannelError Channel::change_mode_o(char flag, int other)
{
	if (flag == '+')
	{
		std::vector<int>::iterator ito = std::find(_operators.begin(), _operators.end(), other);
		if (ito != _operators.end())
		{
			return CHANNEL_OK;
		}
		// ESTA EL MIEMBRO EN CANAL
		std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), other);
		if (it == _members.end())
		{
			return ERR_USER_NOT_IN_CHANNEL;
		}
		// si todo esta bien, añado el cliente a la lista de operators
		_operators.push_back(other);
	}
	else
	{
		// Si todo va bien, elimino ese cliente de la lista de operators
		_operators.erase(std::remove(_operators.begin(), _operators.end(), other),_operators.end());
	}
	return CHANNEL_OK;
}

// limit tiene que ser un int
ChannelError Channel::change_mode_l(char  flag, std::string limit)
{
	if (flag == '+')
	{
		// compruebo que limit sea numerico, no negativo, solo numeros
		for (unsigned int i = 0; i < limit.length(); i++)
		{
			if (limit[i] < '0' || limit[i] > '9')
				return ERR_UNKNOWN_MODE;
		}
		// paso a int y compruebo sus limites
		int limit_int = std::atoi(limit.c_str());
		if (limit_int < 1 || limit_int > 99999)
			return ERR_UNKNOWN_MODE;
		// Cambio el limite al numero introducido
		// Si se cambia por un numero menor, no pasa nada, pero se siguen sin poder
		// añadir usuarios, y el limte sera menor.
		_mode_flag[2] = limit_int;
	}
	else
	{
		// elimino los limites, pasan a -1
		_mode_flag[2] = -1;
	}
	return CHANNEL_OK;
}

ChannelError Channel::change_mode_k(char  flag, std::string password)
{
	if (flag == '+')
	{
		if (password.empty())
			return ERR_NEED_MORE_PARAMS;
		else if (password.length() > 23)
			return ERR_BAD_CHANNEL_KEY;
		// pongo la contraseña, cambio la flag a 1
		_password = password;
		_mode_flag[1] = 1;
	}
	else
	{
		// Pongo password en vacia, flag de modos en 0
		_password = "";
		_mode_flag[1] = 0;
	}
	return CHANNEL_OK;
}

void Channel::change_mode_t(char  flag)
{
	if (flag == '+')
	{
		// cambio la flag a 1
		_mode_flag[3] = 1;
	}
	else
	{
		// flag de modos en 0
		_mode_flag[3] = 0;
	}
}

void Channel::change_mode_i(char flag)
{
	if (flag == '+')
	{
		// cambio la flag a 1
		_mode_flag[0] = 1;
	}
	else
	{
		// flag de modos en 0
		_mode_flag[0] = 0;
	}
}

ChannelError Channel::change_mode(std::string mode, int client, int other_cl, std::string other)
{
	// compruebo que sea un miembro
	std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), client);
	if (it == _members.end())
		return ERR_NOT_ON_CHANNEL;
	// para ejecutar estos modos hay que ser operator
	std::vector<int>::iterator ito = std::find(_operators.begin(), _operators.end(), client);
	if (ito == _operators.end())
		return ERR_NOT_OPERATOR;
	if (mode == "+o" || mode == "-o")
		return change_mode_o(mode[0], other_cl);
	else if (mode == "+l" || mode == "-l")
		return change_mode_l(mode[0], other);
	else if (mode == "+k" || mode == "-k")
		return change_mode_k(mode[0], other);
	else if (mode == "+t" || mode == "-t")
		change_mode_t(mode[0]);
	else if (mode == "+i" || mode == "-i")
		change_mode_i(mode[0]);
	else
		return ERR_UNKNOWN_MODE;
	return CHANNEL_OK;
}

void Channel::print_channel_settings(void)
{
	std::cout << "Members of the channel: ";
	for (size_t i = 0; i < _members.size(); ++i) {
		std::cout << _members[i] << " ";
	}
	std::cout  << std::endl;

	std::cout << "Operators of the channel: ";
	for (size_t i = 0; i < _operators.size(); ++i) {
		std::cout << _operators[i] << " ";
	}
	std::cout  << std::endl;

	for (int i = 0; i < 4; ++i) {
		std::cout << "_mode_flag[" << i << "] = " << _mode_flag[i] << std::endl;
	}

	std::cout << "Password: " << _password << std::endl;
	std::cout << "Name channel: " << _name << std::endl;
}

//GETTERS
std::string Channel::get_password()
{
	return _password;
}

std::string Channel::get_name()
{
	return _name;
}

std::vector<int> Channel::get_members() const
{
	return _members;
}

int *Channel::get_modes()
{
	return _mode_flag;
}

std::vector<int> Channel::get_invite_list()
{
	return _invList;
}

std::string Channel::get_topic()
{
	return _topic;
}

void Channel::add_member(int client, int flag)
{
	std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), client);
	if (it == _members.end())
	{
		_members.push_back(client);
	}
	if (flag == 0)
	{
		std::vector<int>::iterator it = std::find(_operators.begin(), _operators.end(), client);
		if (it == _operators.end())
		{
			_operators.push_back(client);
		}
	}
}

// cambiar/ver topic
ChannelError Channel::change_topic(int cl, std::string new_topic)
{
	// compruebo que sea un miembro
	std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), cl);
	if (it == _members.end())
		return ERR_NOT_ON_CHANNEL;
	// Si no paso nada es solo consulta y si añado es para cambiar el topic
	if(new_topic.empty())
	{
		if (_topic.empty())
		{
			// si lo intento consultar y no hay topic --> eror 331
        	return RPL_NO_TOPIC;
		}
	}
	else
	{
		// consultar flag, si esta a 0 cualquiera puede, si es 1 operators
		if (_mode_flag[3] == 0)
			_topic = new_topic;
		else
		{
			std::vector<int>::iterator it = std::find(_operators.begin(), _operators.end(), cl);
			if (it != _operators.end())
				_topic = new_topic;
			else
			{
				// ERROR -> NO ERES OPERADOR -> MODO 1 -> NO PUEDE CAMBIAR EL TOPIC
				return ERR_NOT_OPERATOR;
			}
		}
	}
	return CHANNEL_OK;
}

// Meto en la list de invitacion 
ChannelError Channel::invite(int cl, int to_inv)
{
	// compruebo usuario a ver si es operator, si lo es -> meto al nuevo invitado
	// si el invitado ya esta invitado, no pasa nada, no se hace nada
	// Si estoy dentro de una canal, ponen a modo +i, y ne voy, no puedo entrar a no ser que me inviten
	std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), cl);
	if (it == _members.end())
		return ERR_NOT_ON_CHANNEL;
	std::vector<int>::iterator ito = std::find(_operators.begin(), _operators.end(), cl);
	if (ito != _operators.end())
	{
		if(_mode_flag[0] != 0)
		// siendo operador, añado, si esta en modo i, sino no hace falta
			_invList.push_back(to_inv);
		return CHANNEL_OK;
	}
	else
		return ERR_NOT_OPERATOR;
}

// Usuario decide irse voluntariamente, el mensaje es opcional
ChannelError  Channel::part(int cl, std::string msg)
{
	(void)msg;
	std::vector<int>::iterator it = std::find(_members.begin(), _members.end(), cl);
	if (it == _members.end())
	{
		// si el usuario es operador puede hacerlo.
		return ERR_USER_NOT_IN_CHANNEL;
	}
	_members.erase(std::remove(_members.begin(), _members.end(), cl),_members.end());
	// Si esta en operators le quito de operators tambien, si no esta no hace nada
	_operators.erase(std::remove(_operators.begin(), _operators.end(), cl), _operators.end());
	return CHANNEL_OK;
}

// Un operador hecha alguien del canal, el mensage es opcional
ChannelError Channel::kick(int cl, int other, std::string msg)
{
	std::vector<int>::iterator it = std::find(_operators.begin(), _operators.end(), cl);
	if (it != _operators.end())
	{
		// si el usuario es operador puede hacerlo.
		return part(other, msg);
	}
	else
		return ERR_NOT_OPERATOR;
}

bool Channel::isOperator(int clientFd) const
{
	return std::find(_operators.begin(), _operators.end(), clientFd) != _operators.end();
}