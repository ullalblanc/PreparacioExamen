#include "Send.h"

Send::Send()
{
}

Send::~Send()
{
}

void Send::SendMessages(sf::IpAddress ip, unsigned short port, char* command, size_t size)
{
	sf::Socket::Status status;
	status = socket->send(command, size, ip, port);
	if (showType)
	{
		InputMemoryBitStream input(command, size);
		int type = 0;
		input.Read(&type, 3);
		//std::cout << type << std::endl;
	}
	//status = socket->send(command->c_str(), command->size(), ip, port);
}