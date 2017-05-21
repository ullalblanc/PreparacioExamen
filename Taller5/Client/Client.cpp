#include <cstring>
#include <iostream>
#include <time.h>

#include "Game.h"

#define SpriteVelocityBot 0.05
#define SpriteVelocityTop 0.05
#define SpriteVelocityBlock 0.07

#define POSICIO_INICIAL1 270
#define POSICIO_INICIAL2 800
#define POSICIO_Y 150

#define DISTANCIA_BODY 70


// Protocol: https://docs.google.com/spreadsheets/d/152EPpd8-f7fTDkZwQlh1OCY5kjCTxg6-iZ2piXvEgeg/edit?usp=sharing

enum State {
	connect,	// Per conectarse al servidor
	send,		// enviar paraula nova y que comenci partida
	play,		// mentres els jugadors estan escribint. comproba si sacaba el temps i si algú ha encertat la partida
	points,		// Envia les puntuacions als jugadors y actualitza els seus logs
	win			// el joc sacaba
};

int main()
{
	bool helloFromServ = false;
	//-- UDP --//

	sf::IpAddress ip = sf::IpAddress::IpAddress("127.0.0.1"); //sf::IpAddress::getLocalAddress();
	unsigned short serverPort = 5000;
	sf::UdpSocket socket;
	std::queue<InputMemoryBitStream> serverCommands;			// Misatges del servidor per anar executant
	std::queue<Command> com;
	sf::Mutex mutex;											// Per evitar varis accesos a les cues
	std::string command;										// el misatge que envia als clients
	Send sender;												// Sender per enviar misatges
	ClientReceive receiver;										// Receiver per rebre constanment misatges
	sf::Thread thread(&ClientReceive::ReceiveCommands, &receiver);	// Thread per el receiver
	std::vector<Player> player;									// Vector de jugadors

	sender.command = &command;
	sender.socket = &socket;

	receiver.commands = &serverCommands;
	receiver.com = &com;
	receiver.socket = &socket;
	receiver.mutex = &mutex;
	receiver.players = &player;

	std::cout << "Port: ";										// Demanem port al client
	unsigned short port;
	std::cin >> port;											// Guardem el port del client
	sf::Socket::Status status = socket.bind(port);				// Bind al port del client
	if (status != sf::Socket::Done) {							// Si falla el bind, acaba el programa
		std::cout << "Error al intent de bind" << std::endl;
		return -1;
	}

	//-- CLIENT --//

	Timer timerConnect;			// timer per intentar conectarse a servidor
	Timer timerAccum;			// timer per el acumulats de moviment
	State state = connect;		// Comencem en connect per que es conecti al server
	Player playertmp;			// Amb el tmp es guardara a ell mateix i als altres en el vector player
	//std::vector<Accum> accum;	// Vector dels acumulats de moviment
	//std::queue<Accum> enemyAccum;// Cua dels acumulats de moviment del contrincant

	playertmp.y = 150;
	player.push_back(playertmp);
	player.push_back(playertmp);
	Accum accumtmp; accumtmp.id = 0;
	//accum.push_back(accumtmp);
	player[0].accum.push_back(accumtmp);

	Bullet bullettmp;
	std::vector<Bullet> bullets;

	//-- GAME --//	

	////-- SPRITES --////

	//Blau
	sf::Texture texture;
	if (!texture.loadFromFile("../Resources/Blue.png")) {
		std::cout << "Can't load the image file" << std::endl;
		return -1;
	}
	sf::Sprite blau; // fons
	blau.setTexture(texture);



	//gespa
	sf::Texture texture2;
	if (!texture2.loadFromFile("../Resources/fucsia.png")) { //FALLA AQUI
		std::cout << "Can't load the image file" << std::endl;
		return -1;
	}
	sf::Sprite fucsia; // fons
	fucsia.setTexture(texture2);
	


	int FoggOffset = 0;
	int DireccioAtacJugador1 = 0; // 0=Idle 1=Top 2=Mid 3=Bot
	int DireccioAtacJugador2 = 0; // 0=Idle 1=Top 2=Mid 3=Bot

	bool distancia = false;
	bool distanciaAtac = false;

	sf::Clock frameClock;//Preparem el temps



	//TEXT
	sf::Font font;
	if (!font.loadFromFile("../Resources/Samurai.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}
	sf::Text text1(/*std::to_string(puntsJugador1)*/"0", font, 50); //Aqui va la variable de puntuacio de cada jugador
	//text1.setColor(sf::Color::White);
	text1.setPosition(150, 750);
	sf::Text text2(/*std::to_string(puntsJugador2)*/"0", font, 50);
	//text2.setColor(sf::Color::White);
	text2.setPosition(1450, 750);
	sf::Text PointText("", font, 100);
	PointText.setPosition(300, 250);
	sf::Text Instructions("Prem 'ENTER' per iniciar la propera ronda", font, 30);
	Instructions.setPosition(300, 750);

	sf::Vector2i screenDimensions(512,512);											// Dimensions pantalles
	sf::RenderWindow window;															// Creem la finestra del joc
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Aoi Samurai");	// Obrim la finestra del joc
	window.setFramerateLimit(60); //FrameRate

	thread.launch();																	// Comencem thread receive

	int left = 0;
	int right = 1;

	while (window.isOpen())
	{
	
		sf::Event event; //Si no la finestra no detecta el ratolí i no es pot moure
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
				window.close();
		}

		sf::Time frameTime = frameClock.restart();
		switch (state) {
		case connect: {

			if (timerConnect.Check()) {
				if (player[0].x == 0) {
					OutputMemoryBitStream output;
					output.Write(HELLO, TYPE_SIZE);
					sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
					std::cout << "enviem Hello" << std::endl;
					//timerConnect.Stop();
				}
				timerConnect.Start(5000);
			}

			if (!com.empty()) {
				std::cout << "rebem en el connect" << std::endl;
				switch (com.front().type) {
					
				case HELLO: {
					std::cout << "rebem en el connect; hello" << std::endl;
					player[0].id = com.front().id;
					player[0].y = com.front().position;
					helloFromServ = true;

					if (player[0].id == 1)
					{
						left = 1;
						right = 0;
					}

					com.pop();
				}
					break;

				case CONNECTION: {
					std::cout << "rebem en el connect: Conexio" << std::endl;
					player[1].id = com.front().id;
					player[1].y = com.front().position;


					com.pop();

				}
					break;
				}
			}
			if (player[0].y != 0 && helloFromServ == true/*&& player[1].x != 0*/)
			{
				OutputMemoryBitStream output;
				output.Write(CONNECTION, TYPE_SIZE);
				output.Write(player[0].id, ID_SIZE);
				sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
				state = play;
				//std::cout << "play " << player[0].x << " " << player[1].x << std::endl;
			}
		}
			break;

		case send: {}
			break;

		case play: {

			//-- MOVEMENT --//

			sf::Keyboard key;
			if (key.isKeyPressed(sf::Keyboard::Right)) {
				int movement = 2;
				//int distance = player[1].x - (player[0].x + movement);
				//if (distance < 0) distance = -distance;
				//if (distance > DISTANCIA_BODY)
				//{
					if ((player[0].y + movement) < RIGHT_LIMIT)
					{
						player[0].y += movement;
						player[0].accum.back().delta += movement;
						
					}
				//}
				// TODO: vigilar que no xoquin amb el enemic
				//p1Bot.play(pas1B);

			}
			if (key.isKeyPressed(sf::Keyboard::Left)) {
				int movement = -2;
				//int distance = player[1].y - (player[0].y + movement);
				//if (distance < 0) distance = -distance;
				
					if ((player[0].y + movement) > LEFT_LIMIT)
					{
						
						player[0].y += movement;
						player[0].accum.back().delta += movement;
						
					}
				
				//p1Top.play(attackAnimationTop1T);
			}

			if (key.isKeyPressed(sf::Keyboard::Z) && player[0].attack == 0) {
				if (!left) {
					
				}
				else {
					
				}
				player[0].attack = 1;

				OutputMemoryBitStream output;
				output.Write(ATTACK, TYPE_SIZE);
				output.Write(player[0].id, ID_SIZE);
				output.Write(player[0].attack, ATTACK_SIZE);

				sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
			}

			if (key.isKeyPressed(sf::Keyboard::X) && player[0].attack == 0) {
				if (!left) {
				
				}
				else {
				
				}
				player[0].attack = 2;

				OutputMemoryBitStream output;
				output.Write(ATTACK, TYPE_SIZE);
				output.Write(player[0].id, ID_SIZE);
				output.Write(player[0].attack, ATTACK_SIZE);

				sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
			}

			if (key.isKeyPressed(sf::Keyboard::C) && player[0].attack == 0) {
				if (!left) {
			
				}
				else {

				}
				player[0].attack = 3;

				OutputMemoryBitStream output;
				output.Write(ATTACK, TYPE_SIZE);
				output.Write(player[0].id, ID_SIZE);
				output.Write(player[0].attack, ATTACK_SIZE);

				sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
			}

			if (player[0].attack != 0)
			{
				if (!left) 
				{
					if (false) {
						OutputMemoryBitStream output;
						output.Write(ATTACK, TYPE_SIZE);
						output.Write(player[0].id, ID_SIZE);
						output.Write(player[0].attack, ATTACK_SIZE);

						sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
						player[0].attack = 0;
					}
					
				}
				else
				{
					if (false) {
						OutputMemoryBitStream output;
						output.Write(ATTACK, TYPE_SIZE);
						output.Write(player[0].id, ID_SIZE);
						output.Write(player[0].attack, ATTACK_SIZE);

						sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
						player[0].attack = 0;
					}
					
				}
				
			}
			// TODO: Attack

			//-- ACCUMULATED --//

			////-- PLAYER --////

			if (timerAccum.Check())
			{
				if (player[0].accum.back().delta != 0)
				{
					//int negative = 0; // 0 = positiu, 1 = negatiu
					if (player[0].accum.back().delta < 0) {
						player[0].accum.back().sign = 1;
						player[0].accum.back().delta = -player[0].accum.back().delta;
					}

					player[0].accum.back().absolute = player[0].x;			// Marco el absolut del moviment
					OutputMemoryBitStream output;
					output.Write(MOVEMENT, TYPE_SIZE);
					output.Write(player[0].id, ID_SIZE);
					output.Write(player[0].accum.back().id, ACCUM_ID_SIZE);
					output.Write(player[0].accum.back().sign, ID_SIZE);
					output.Write(player[0].accum.back().delta, ACCUM_DELTA_SIZE);
					output.Write(player[0].accum.back().absolute, POSITION_SIZE);

					//std::cout << "Enviat " << player[0].accum.back().delta << std::endl;

					sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());

					Accum accumtmp;										// Creo nou acumulat
					if (player[0].accum.back().id == 15) accumtmp.id = 0;	// Si el ultim acumulat te id 15, el nou torna a 0
					else accumtmp.id = player[0].accum.back().id + 1;     // Sino, el id es un mes que l'anterior
					
					player[0].accum.push_back(accumtmp);
				}	
				timerAccum.Start(ACCUMTIME);
			}

			////-- ENEMY --////

			if (!player[1].accum.empty())
			{
				int movement = 2;
				if (player[1].accum.front().absolute != player[1].x)
				{
					if (player[1].accum.front().delta < 0) movement = -movement;
					player[1].x += movement;
				}
				else
				{
					player[1].accum.erase(player[1].accum.begin());
				}
			}

			//-- COMMANDS --// 
			//Apliquem fisiques de les bales.
			if (bullets.size() > 0) {
				//std::cout << "Entrem al size" << std::endl;
				for (int i = 0; i < bullets.size(); i++)
				{
					
					bullets[i].x -= 10;
					if (bullets[i].x == player[left].x) { //Enviem al servidor per evaluar si xoquen.

					}
				}

			}
			
			if (!com.empty()) {


				std::cout << "Rebem quelcom" << std::endl;
				//int serverCase = 0; 
				//serverCommands.front().Read(&serverCase, TYPE_SIZE);
				switch (com.front().type) {

				case HELLO: { // NO TINDRIA QUE REBRE 1
					std::cout << "rebem hello" << std::endl;
					helloFromServ = true;
					
				}
					break;

				case CONNECTION: {
					OutputMemoryBitStream output;
					output.Write(CONNECTION, TYPE_SIZE);
					output.Write(player[0].id, ID_SIZE);
					sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
					com.pop();
					std::cout << "Rep conexio" << std::endl;
					break;
				}
				case PING: {
					OutputMemoryBitStream output;
					output.Write(PING, TYPE_SIZE);
					output.Write(player[0].id, ID_SIZE);
					sender.SendMessages(ip, serverPort, output.GetBufferPtr(), output.GetByteLength());
					com.pop();
					std::cout << "Envio i rebo ping" << std::endl;
					break;
				}
				case ENEMY: {
					std::cout << "Rebem Enemics" << std::endl;

					srand(time(NULL));
					int randNum = rand() % 10;
					randNum = randNum * 51;
					std::cout << randNum << std::endl;
					//bullettmp.y = com.front().position;

					bullettmp.y = randNum;
					//std::cout << bullettmp.y << std::endl;

					bullets.push_back(bullettmp);
					

				}
				case MOVEMENT: {
					std::cout << "rebem moviment" << std::endl;
					if (com.front().id == player[0].id)				// Si es el id propi, comfirma el moviment
					{	// TODO: Check de trampas o problemes
						for (int i = 0; i < player[0].accum.size(); i++)	// Recorre tots els misatges de acumulacio
						{
							if (player[0].accum[i].id == com.front().accum.id)		// Si troba el misatge de acumulacio
							{
								for (int j = 0; j < player[0].accum.size()-i; j++)		// Recorre els misatges que hi havien fins ara
								{
									player[0].accum.erase(player[0].accum.begin());					// Borrals
								}
								break;
							}
						}
					} 
					else							// Si es el id del contrincant, simula el moviment
					{
						Accum accumtmp = com.front().accum;
						//accumtmp.absolute = player[1].x + accumtmp.delta;
						player[1].accum.push_back(accumtmp);	// Afegir acumulat a la cua
					}

					com.pop();
					//serverCommands.pop();					
				}
				break;

				case ATTACK:
				{
					player[1].attack = com.front().position;
					if (!left) {
						if (player[1].attack == 1)
						{

						}
						else if (player[1].attack == 2)
						{

						}
						else if (player[1].attack == 3)
						{

						}												
					}
					else {
						if (player[1].attack == 1)
						{

						}
						else if (player[1].attack == 2)
						{

						}
						else if (player[1].attack == 3)
						{

						}
					}
					com.pop();
				}
				break;

				case SCORE:
				{
					player[com.front().id].score++;
					
					text1.setString(std::to_string(player[0].score));
					text2.setString(std::to_string(player[1].score));

					if (!left)
					{
						player[0].x = 270;
						player[1].x = 800;
					} 
					else 
					{
						player[1].x = 270;
						player[0].x = 800;
					}					

					player[0].attack = 0;
					player[1].attack = 0;


					timerAccum.Start(ACCUMTIME);

					for (int j = 0; j < player.size(); j++)
					{
						for (int i = 0; i < player[j].accum.size(); i++)
						{
							player[j].accum.erase(player[j].accum.begin());
						}					
					}				
					Accum accumtmp;
					player[0].accum.push_back(accumtmp);

					com.pop();
				}
				break;

				default:
					break;

				}
			}
		}
			break;
		}

		

		//if (player.size() > 0) { 
		//p1Bot.play(*currentAnimation1B);

		//Apliquem fisiques



		blau.setPosition(player[left].x, player[left].y);
		window.draw(blau);
		//p1Top.play(*currentAnimation1T);

		if (bullets.size()>0) {
			for (int i = 0; i < bullets.size(); i++)
			{
				fucsia.setPosition(bullets[i].x, bullets[i].y);
				window.draw(fucsia);
			}
		}
	
	


		window.draw(text1); //Text de puntuacions
		window.draw(text2);

		if (state == points || state == win) { //Pintem el text si el estat es point o win
			window.draw(PointText);
			window.draw(Instructions);
		}

		window.display();		// Mostrem per la finestra
		window.clear();			// Netejem finestra
	}
	receiver.stopReceive = false;
	thread.terminate();
	return 0;
}