#include <cstring>

#include "Game.h"

// Protocol: https://docs.google.com/spreadsheets/d/152EPpd8-f7fTDkZwQlh1OCY5kjCTxg6-iZ2piXvEgeg/edit?usp=sharing

enum State {
	connect,	// Esperar a que es conectin els dos jugadors
	play,		// Partida
	points,		// Un jugador mata a l'altre i se li suma un punt
	win			// el joc sacaba
};

int main()
{
	//-- UDP --//

	sf::IpAddress ip = sf::IpAddress::IpAddress("127.0.0.1");	// sf::IpAddress::getLocalAddress();
	sf::UdpSocket socket;										// El socket del servidor
	std::queue<InputMemoryBitStream> clientCommands;			// Misatges dels jugadors per anar executant
	std::queue<Command> com;
	sf::Mutex mutex;											// Per evitar varis accesos a les cues
	std::string command;										// el misatge que envia als clients
	Send sender;												// Sender per enviar misatges
	ServerReceive receiver;										// Receiver per rebre constanment misatges
	sf::Thread thread(&ServerReceive::ReceiveCommands, &receiver);	// Thread per el receiver
	std::vector<ServerPlayer> player;							// Vector de jugadors
	ServerPlayer playertmp;

	sf::Socket::Status status = socket.bind(5000);				// Bind al port 5000
	if (status != sf::Socket::Done) {
		std::cout << "Error al intent de bind" << std::endl;
		return -1;
	}
	socket.setBlocking(false);									// Fiquem socketa non Blocking

	sender.command = &command;
	sender.socket = &socket;

	receiver.commands = &clientCommands;
	receiver.com = &com;
	receiver.socket = &socket;
	receiver.mutex = &mutex;
	receiver.players = &player;

	//-- SERVER --//

	MessageManager protocol;
	Timer timerReady;
	Timer timerPing;
	Timer timerSpawn;
	State state = connect;
	bool playersConected = false;

	timerReady.Start(0);
	timerPing.Start(0);
	timerSpawn.Start(0);

	Bullet bullettmp;
	std::vector<Bullet> bullets;

	//-- GAME --//

	bool gameOn = true;
	thread.launch();

	std::cout << "Server";

	while (gameOn)
	{
		sf::Keyboard key;
		if (key.isKeyPressed(sf::Keyboard::BackSpace)) { // si el server vol tancar la comunicacio
			state = win;
		}
		mutex.lock();
		switch (state) {

	//-- CONECT --//
		case connect: { // que es conectin els dos jugadors
			if (!playersConected) {
				
				//mutex.lock();
				if (!com.empty()) {
					switch (com.front().type) {
					case HELLO: {	// Un client es vol conectar

						for (int i = 0; i < 1/*player.size()*/; i++)
						{
							if (com.front().id == player[i].id) {	// el jugador que diu Hello
								if (player[i].y <= 0) {		// necesita posicio
									if (player[i].id == 0) {
										player[i].y = 1;// jugador 1 a 270
									}
									else {
										player[i].y = 800; // jugador 2 a 1330
									}
									player[i].x = 1;
									std::cout << "\n New user" << std::endl;
								}
								OutputMemoryBitStream output;
								output.Write(HELLO, TYPE_SIZE);
								output.Write(player[i].id, ID_SIZE);
								output.Write(player[i].x, POSITION_SIZE);
								sender.SendMessages(player[i].ip, player[i].port, output.GetBufferPtr(), output.GetByteLength());
								//clientCommands.pop();
								com.pop();
							}
						}
					}
						break;
					}
				}
				if (player.size() == TOTALPLAYERS) { // Si existeixen 2 jugadors
					std::cout << "1" << std::endl;
					if (player[0].y > 0 /*&& player[1].x > 0*/) {// si els 2 jugadors tenen posicions valides, estan correctament conectats
						playersConected = true;
						std::cout << "2" << std::endl;
						for (int j = 0; j < com.size(); j++)
						{
						
							com.pop();
							//clientCommands.pop();
						}
					}
				}
				//mutex.unlock();
			}
			else
			{
				if (timerReady.Check()) {
					std::cout << "3" << std::endl;
					OutputMemoryBitStream output;
					output.Write(CONNECTION, TYPE_SIZE);
					output.Write(player[0].id, ID_SIZE);
					output.Write(player[0].x, POSITION_SIZE);
					sender.SendMessages(player[1].ip, player[1].port, output.GetBufferPtr(), output.GetByteLength());
					std::cout << "Enviem conexio" << std::endl;
					for (int i = 0; i < player[1].keyComs.size(); i++)
					{
						if (player[1].keyComs[i].type == CONNECTION) {
							break;
						}
						else if (i == player[1].keyComs.size() - 1) {
							Command comtmp;
							comtmp.type = CONNECTION;
							player[1].keyComs.push_back(comtmp);
						}
					}
					OutputMemoryBitStream output2;
					output2.Write(CONNECTION, TYPE_SIZE);
					output2.Write(player[1].id, ID_SIZE);
					output2.Write(player[1].x, POSITION_SIZE);
					sender.SendMessages(player[0].ip, player[0].port, output2.GetBufferPtr(), output2.GetByteLength());
					for (int i = 0; i < player[0].keyCommands.size(); i++)
					{
						if (player[0].keyComs[i].type == CONNECTION) {
							break;
						}
						else if (i == player[0].keyComs.size() - 1) {
							Command comtmp;
							comtmp.type = CONNECTION;
							player[0].keyComs.push_back(comtmp);
						}
					}
					timerReady.Start(5000);
				}
				//mutex.lock();
				if (!com.empty()) {
					switch (com.front().type) {
					case HELLO:
						playersConected = false;
						std::cout << "Rebem Hello" << std::endl;
						//clientCommands.pop();
						break;
					case CONNECTION:	// Un client es vol conectar
						std::cout << "Rebem connection" << std::endl;
						player[com.front().id].ready = 1;
						for (int i = 0; i < player[com.front().id].keyComs.size(); i++) // Recorrer tots els keycommands
						{
							if (player[com.front().id].keyComs[i].type == CONNECTION) {								// si es un keycommand de ready							
								for (int j = 0; j < player[com.front().id].keyComs.size(); j++)
								{
									player[com.front().id].keyComs.erase(player[com.front().id].keyComs.begin() + j);	// borral
									//std::cout << "All messages deleted for player " << com.front().id << std::endl;
								}							
								break;
							}
						}
						com.pop();
						break;
					}
				}
				//mutex.unlock();
				if (player[0].ready == 1 /*&& player[1].ready == 1*/)
				{
					std::cout << "Go to play" << std::endl;
					state = play;
				}
			}
		}
			break;

	//-- PLAY --//

		case play: {
			//mutex.lock();
			////-- PING --////

			if (timerPing.Check()) {
				// Check si un o els dos jugadors s'ha desconectat
				bool toConect = false;
				for (int i = 0; i < TOTALPLAYERS; i++)
				{
					std::cout << "Ping" << std::endl;
					for (int j = 0; j < player[i].keyComs.size(); j++)
					{
						if (player[i].keyComs[j].type == PING) {
							OutputMemoryBitStream output;
							output.Write(DISCONNECTION, TYPE_SIZE);
							output.Write(i, ID_SIZE); // Misatge que s'ha desconectat el jugador i

							//std::cout << "player " << i << " seems to be disconected" << std::endl;

							for (int k = 0; k < 1/*player.size()*/; k++)
							{
								sender.SendMessages(player[k].ip, player[k].port, output.GetBufferPtr(), output.GetByteLength());
								std::cout << "Enviem" << std::endl;
							}

							player.erase(player.begin() + i);
							toConect = true;
						}
					}
				}
				if (toConect) { // si hi ha algun jugador desconectat
					state = connect;
				}
				else {
					OutputMemoryBitStream output;
					output.Write(PING, TYPE_SIZE);
					bool foundMessage = false;									// Per saber si hi ha un misatge igual
					for (int k = 0; k < player.size(); k++)
					{
						sender.SendMessages(player[k].ip, player[k].port, output.GetBufferPtr(), output.GetByteLength());
						for (int l = 0; l < player[k].keyComs.size(); l++)
						{
							if ( player[k].keyComs[l].type == PING) {
								foundMessage = true;
								break;
							}
						}
						if (!foundMessage)
						{
							Command com;
							com.type = PING;
							player[k].keyComs.push_back(com);
						}

						foundMessage = false;
					}
					
					timerPing.Start(3000);
				}

			}
			if (timerSpawn.Check()) {
				std::cout << "Spamejemen un enemic" << std::endl;

				srand(time(NULL));
				int randNum = rand() % (472 - 0 + 1) + 0;
				std::cout << randNum << std::endl;

				OutputMemoryBitStream outputEnemy;
				outputEnemy.Write(ENEMY, TYPE_SIZE);
				//outputEnemy.Write(randNum, POSITION_SIZE);
				sender.SendMessages(player[0].ip, player[0].port, outputEnemy.GetBufferPtr(), outputEnemy.GetByteLength());

				timerSpawn.Start(500);
				
			}

			////-- CLIENT COMMANDS --////

			
			if (!com.empty()) {
				
				//std::cout << "And now Client Case is " << com.front().type << std::endl;

				switch (com.front().type) {

				case HELLO: {	// Un client es vol conectar
					state = connect;
					com.pop();
				}
					break;

				case CONNECTION: {
					com.pop();
				}
					break;

				case PING: {
					//std::cout << "Player " << com.front().id << " Pinged" << std::endl;
					for (int i = 0; i < player[com.front().id].keyComs.size(); i++)
					{
						if (player[com.front().id].keyComs[i].type == PING) {
							player[com.front().id].keyComs.erase(player[com.front().id].keyComs.begin());
							break;
						}
					}
					com.pop();

				}
					break;

				case DISCONNECTION: {
					OutputMemoryBitStream output;
					output.Write(PING, TYPE_SIZE);
					output.Write(com.front().id, ID_SIZE);
					//bool foundMessage = false;									// Per saber si hi ha un misatge igual
					for (int k = 0; k < player.size(); k++)
					{
						sender.SendMessages(player[k].ip, player[k].port, output.GetBufferPtr(), output.GetByteLength());
					}
					player.erase(player.begin() + com.front().id);
					state = connect;
					com.pop();

				}
									break;
				case MOVEMENT: {
					Accum accumtmp = com.front().accum;
					std::cout << "rebem moviment" << std::endl;
					int threshold = 6; // Maximum error able to forgive
					for (int i = -threshold; i < threshold; i++)
					{
						//if (accumtmp.absolute > LEFT_LIMIT && accumtmp.absolute < RIGHT_LIMIT) {
							if ((player[com.front().id].y + accumtmp.delta + i) == accumtmp.absolute) { // si esta dins de la posicio que estem disposats a accpetar
								player[com.front().id].y = accumtmp.absolute; //Aqui hem canviat xy
								player[com.front().id].accum.push_back(accumtmp);
								std::cout << "es mou" << std::endl;
								OutputMemoryBitStream output;
								output.Write(MOVEMENT, TYPE_SIZE);
								output.Write(com.front().id, ID_SIZE);
								output.Write(player[com.front().id].accum.back().id, ACCUM_ID_SIZE);
								output.Write(player[com.front().id].accum.back().sign, ID_SIZE);
								if (player[com.front().id].accum.back().delta < 0)
									output.Write(-player[com.front().id].accum.back().delta, ACCUM_DELTA_SIZE);
								else
									output.Write(player[com.front().id].accum.back().delta, ACCUM_DELTA_SIZE);
								output.Write(player[com.front().id].accum.back().absolute, POSITION_SIZE);

								//std::cout << "Delta received: " << com.front().accum.delta << std::endl;

								for (int k = 0; k < player.size(); k++)
								{
									std::cout << "enviem moviment" << std::endl;
									sender.SendMessages(player[k].ip, player[k].port, output.GetBufferPtr(), output.GetByteLength());
								}
							}
						//}
					}
					//accumtmp.absolute = player[com.front().id].x + accumtmp.delta;
					// TODO: Comprobacions de trampas i limits

					com.pop();
					
				}
							   break;
				case ATTACK: {

					if (player[com.front().id].attack == 0)
					{
						player[com.front().id].attack = com.front().position;

						OutputMemoryBitStream output;
						output.Write(ATTACK, TYPE_SIZE);
						output.Write(player[com.front().id].id, ID_SIZE);
						output.Write(player[com.front().id].attack, ATTACK_SIZE);

						if (com.front().id == 0)
							sender.SendMessages(player[1].ip, player[1].port, output.GetBufferPtr(), output.GetByteLength());
						else 
							sender.SendMessages(player[0].ip, player[0].port, output.GetBufferPtr(), output.GetByteLength());

					}
					else {
						int distance = player[0].x - player[1].x;
						if (distance < 0) distance = -distance;
						if (distance < DISTANCE_ATTACK)
						{
							player[com.front().id].score++;

							player[0].x = 270;
							player[1].x = 800;

							player[0].attack = 0;
							player[1].attack = 0;

							OutputMemoryBitStream output;
							output.Write(SCORE, TYPE_SIZE);
							output.Write(player[com.front().id].id, ID_SIZE);

							for (int k = 0; k < player.size(); k++)
							{
								sender.SendMessages(player[k].ip, player[k].port, output.GetBufferPtr(), output.GetByteLength());
							}
						}
					}
					com.pop();
				}
					break;
				}
			}
		}
			break;
		}
		mutex.unlock();
	}
	receiver.stopReceive = false;
	thread.terminate();
	return 0;
}