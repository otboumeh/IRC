#pragma once
#include <string>
#include <vector>

class Command{
private:
	std::string _command;
	std::vector<std::string> _params;
public:

	Command(const std::string& rawCommand);
	const std::string& getCommand() const;
	const std::vector<std::string>& getParams() const;
};