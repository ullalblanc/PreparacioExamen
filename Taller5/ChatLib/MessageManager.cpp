#include "MessageManager.h"


// 0=null

// _auxText[0] -> Type
// _auxText[1] -> Subtype
// _auxText[2] -> Player
// _auxText[3..] -> Message

// Protocol: https://docs.google.com/spreadsheets/d/152EPpd8-f7fTDkZwQlh1OCY5kjCTxg6-iZ2piXvEgeg/edit?usp=sharing

//Funciones std::String
//http://www.cplusplus.com/reference/string/string/operator[]/
//http://www.cplusplus.com/reference/string/string/replace/
//http://www.cplusplus.com/reference/string/string/clear/
//http://www.cplusplus.com/reference/string/string/erase/



int MessageManager::GetType(std::string RawMessage)
{
	std::string _auxText = "";
	_auxText = RawMessage;
	int _auxInt = _auxText[0];
	_auxInt = _auxInt - 48;
	return _auxInt;
}

int MessageManager::GetSubType(std::string RawMessage)
{
	std::string _auxText = "";
	_auxText = RawMessage;
	int _auxInt = _auxText[1];
	_auxInt = _auxInt - 48;
	return _auxInt;
}

int MessageManager::GetFirst(std::string RawMessage)
{
	std::string _auxText = "";
	_auxText = RawMessage;
	int _auxInt = _auxText[2];
	_auxInt = _auxInt - 48;
	return _auxInt;
}


int MessageManager::GetSecond(std::string RawMessage)
{
	std::string _auxText = "";
	_auxText = RawMessage;
	int _auxInt = _auxText[3];
	_auxInt = _auxInt - 48;
	return _auxInt;
}

int MessageManager::GetPosition(std::string RawMessage)
{
	std::string _auxText = "";
	_auxText = RawMessage;
	int _auxInt = (_auxText[2] - 48) * 1000 + (_auxText[3]-48)*100 + (_auxText[4] - 48) * 10 + (_auxText[5] - 48);
	return _auxInt;
}

std::string MessageManager::CreateMessage(int type, int subtype, int first, int second)
{
	std::string message = "";
	message = std::to_string(type) + std::to_string(subtype) + std::to_string(first) + std::to_string(second);
	return message;
}

std::string MessageManager::CreateMessage(std::vector<int> messageArray)
{
	std::string message = "";
	for (int i = 0; i < messageArray.size(); i++)
	{
		message = message + std::to_string(messageArray[i]);
	}
	return message;
}

std::string MessageManager::CreateMessageP(int type, int subtype, int position)
{
	std::string message = "";
	if (position - 1000 < 0) {
		message = std::to_string(type) + std::to_string(subtype) + "0" + std::to_string(position);
	}
	else {
		message = std::to_string(type) + std::to_string(subtype) + std::to_string(position);
	}
	return message;
}

MessageManager::MessageManager()
{
}


MessageManager::~MessageManager()
{
}
