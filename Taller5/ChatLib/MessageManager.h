#pragma once
#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <string>
#include <iostream>

class MessageManager
{
private:
	std::string _auxText = "";


public:

	int GetType(std::string RawMessage);
	int GetSubType(std::string RawMessage);
	int GetFirst(std::string RawMessage);
	int GetSecond(std::string RawMessage);
	int GetPosition(std::string RawMessage);
	std::string CreateMessage(int type, int subtype, int first, int second);
	std::string CreateMessage(std::vector<int> messageArray);
	std::string CreateMessageP(int type, int subtype, int position);

	MessageManager();
	~MessageManager();
};
