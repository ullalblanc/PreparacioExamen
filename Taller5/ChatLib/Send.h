#pragma once

#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <iostream>

#include "InputMemoryBitStream.h"
#include "OutputMemoryBitStream.h"

class Send
{
public:
	Send();
	~Send();

	void SendMessages(sf::IpAddress ip, unsigned short port, char* command, size_t size);

	bool showType = true;
	sf::UdpSocket *socket;
	std::string *command;
};