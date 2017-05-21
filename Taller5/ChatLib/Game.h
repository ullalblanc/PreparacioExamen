#pragma once
#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <string>
#include <iostream>
#include <queue>
#include <vector>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include "Send.h"
#include "MessageManager.h"
#include "InputMemoryBitStream.h"
#include "OutputMemoryBitStream.h"

#define MAXTIME 10000 
#define ACCUMTIME 100 //Temps per acumular delta
#define TOTALPLAYERS 1
#define LEFT_LIMIT 0
#define RIGHT_LIMIT 472 //512 -40
#define PLAYER_LIMIT 70
#define DISTANCE_ATTACK 200

#define TYPE_SIZE 3 
#define ID_SIZE 1 // Tambien funciona para negatio i positivo
#define POSITION_SIZE 12
#define ACCUM_ID_SIZE 4
#define ACCUM_DELTA_SIZE 6 // de -64 a +64
#define ATTACK_SIZE 2

enum Type { // uint2
	HELLO,			// Un jugador es vol conectar
	CONNECTION,		// Informacio sobre conexio
	PING,			// PING
	DISCONNECTION,	// Avis de desconexio
	MOVEMENT,		// Informació sobre el moviment
	ATTACK,			// Informació sobre el atac
	SCORE,			// Informació sobre la puntuació
	ENEMY,			//Spamejem Enemics
};

/*Paquets de acumulacio de moviment*/
struct Accum {
	int id = 0;
	int sign = 0;
	int delta = 0;
	int absolute = 0;
};

struct Command {
	int type = 0;
	int id = 0;
	int position = 0;
	Accum accum;
};

/*Clase Player que guarda la informació basica de un jugador
Només per Client*/
class Player {

public:
	int id = 0;
	int x = 0;
	int y;
	int ready = 0;
	int attack = 0; // 0=Idle 1=Top 2=Mid 3=Bot
	int score = 0;
	//std::vector<std::string> keyCommands;
	std::vector<OutputMemoryBitStream> keyCommands;
	std::vector<Command> keyComs;
	std::vector<Accum> accum;
};

class Bullet {

public:
	int id = 0;
	int x = 512;
	int y = 0;

};
class ServerPlayer : public Player {
	
public:

	sf::IpAddress ip;
	unsigned short port;

};



class Timer {
	std::clock_t time = std::clock();
	bool stopped = false;
public:
	Timer() {};
	~Timer() {};

	void Start(float maxtime) {
		time = std::clock() + maxtime;
		stopped = false;
	};

	void Stop() {
		//time = 0;
		stopped = true;
	};

	bool Check() {
		if (!stopped) {
			if (time < std::clock()) return true; //Torna True al Acabar
			else return false;
		}
		return false;
	};

	int GetTime() {
		return (int)((time - std::clock())/1000);
	};
};

class Receive
{
public:
	sf::UdpSocket *socket;
	std::queue<InputMemoryBitStream> *commands;
	std::queue<Command> *com;
	sf::Mutex *mutex;
	bool stopReceive = true;

	Receive() {};
	~Receive() {};

	virtual void ReceiveCommands() = 0;
};

class ServerReceive : public Receive {

public:
	std::vector<ServerPlayer> *players;

	void ReceiveCommands() {
		do {
			char data[1500];
			std::size_t received;
			sf::IpAddress ip;
			unsigned short port;

			sf::Socket::Status status = socket->receive(data, 1500, received, ip, port);
			data[received] = '\0';

			if (status == sf::Socket::Done) {
				mutex->lock();
				//commands->push(data);
				InputMemoryBitStream newCommand(data, received * 8);
				commands->push(newCommand);

				Command comtmp;
				comtmp.type = 0;
				newCommand.Read(&comtmp.type, 3);

				//std::cout << "Client Case is " << comtmp.type << std::endl;

				switch (comtmp.type) {

				case HELLO: {
					ServerPlayer playertmp;
					if (!players->empty()) {
						for (int i = 0; i < players->size(); i++) // recorre tots els jugadors
						{
							if (players->at(i).port == port && players->at(i).ip == ip) // Si es el jugador 2
							{
								comtmp.id = players->at(i).id;
								break;							// acaba el for
							}
							else if (i == players->size() - 1) {	// si no existeix 
								playertmp.ip = ip;				// crea nou jugador
								playertmp.port = port;
								if (players->at(0).id == 0)
								{
									playertmp.id = 1; // player 2
								}
								else
								{
									playertmp.id = 0; // player 1
								}
								players->push_back(playertmp);
								comtmp.id = players->at(i).id; // Marca per donar posicio
							}
						}
					}
					else {
						playertmp.ip = ip;					// crea nou jugador
						playertmp.port = port;
						playertmp.id = 0;

						players->push_back(playertmp);
						comtmp.id = playertmp.id; // Marca per donar posicio
					}
				}
							break;
				case CONNECTION :
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;

				case PING :
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;

				case DISCONNECTION:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;

				case MOVEMENT:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);			// ID del player
					newCommand.Read(&comtmp.accum.id, ACCUM_ID_SIZE);		// ID del acumulat
					newCommand.Read(&comtmp.accum.sign, ID_SIZE);	// Signe del acumulat
					newCommand.Read(&comtmp.accum.delta, ACCUM_DELTA_SIZE);	// Accumulat
					newCommand.Read(&comtmp.accum.absolute, POSITION_SIZE);	// Absolut

					//std::cout << " Delta " << comtmp.accum.delta << std::endl;

					if (comtmp.accum.sign == 1) comtmp.accum.delta = -comtmp.accum.delta; // Canvia de signe acumulat si requereix
				}
				break;

				case ATTACK:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
					newCommand.Read(&comtmp.position, ATTACK_SIZE); // quin atac a fet. // 0=Idle 1=Top 2=Mid 3=Bot
				}
				break;

				case SCORE:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;

				}
				com->push(comtmp);
				mutex->unlock();
			}
		} while (stopReceive);
	};
};

class ClientReceive : public Receive {

public:
	std::vector<Player> *players;

	void ReceiveCommands() {
		do {
			char data[1500];
			std::size_t received;
			sf::IpAddress ip;
			unsigned short port;

			sf::Socket::Status status = socket->receive(data, 1500, received, ip, port);
			data[received] = '\0';

			
			if (status == sf::Socket::Done) {
				mutex->lock();
				InputMemoryBitStream newCommand(data, received * 8);
				commands->push(newCommand);

				Command comtmp;
				comtmp.type = 0;
				newCommand.Read(&comtmp.type, 3);

				switch (comtmp.type) {

				case HELLO: {
					newCommand.Read(&comtmp.id, ID_SIZE);
					newCommand.Read(&comtmp.position, POSITION_SIZE);
				}
				break;

				case CONNECTION:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
					newCommand.Read(&comtmp.position, POSITION_SIZE);
				}
				break;

				case PING:
				{
				}
				break;

				case DISCONNECTION:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;
				
				case ENEMY: {

					newCommand.Read(&comtmp.accum.absolute, POSITION_SIZE);	// Absolut
				}

				case MOVEMENT:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);			// ID del player
					newCommand.Read(&comtmp.accum.id, ACCUM_ID_SIZE);		// ID del acumulat
					newCommand.Read(&comtmp.accum.sign, ID_SIZE);	// Signe del acumulat
					newCommand.Read(&comtmp.accum.delta, ACCUM_DELTA_SIZE);	// Accumulat
					newCommand.Read(&comtmp.accum.absolute, POSITION_SIZE);	// Absolut

					if (comtmp.accum.sign == 1) comtmp.accum.delta = -comtmp.accum.delta; // Canvia de signe acumulat si requereix
					//std::cout << " Delta " << comtmp.accum.delta << std::endl;
				}
				break;

				case ATTACK:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
					newCommand.Read(&comtmp.position, ATTACK_SIZE); // quin atac a fet. // 0=Idle 1=Top 2=Mid 3=Bot
				}
				break;

				case SCORE:
				{
					newCommand.Read(&comtmp.id, ID_SIZE);
				}
				break;

				}
				com->push(comtmp);
				mutex->unlock();
			}
		} while (stopReceive);
	};
};

// Copyright (C) 2014 Maximilian Wagenbach (aka. Foaly) (foaly.f@web.de)
// https://github.com/SFML/SFML/wiki/Source:-AnimatedSprite
class Animation
{
public:
	Animation() : m_texture(NULL){};

	void addFrame(sf::IntRect rect) {
		m_frames.push_back(rect);
	};
	void setSpriteSheet(const sf::Texture& texture) {
		m_texture = &texture;
	};
	const sf::Texture* getSpriteSheet() const {
		return m_texture;
	};
	std::size_t getSize() const {
		return m_frames.size();
	};
	const sf::IntRect& getFrame(std::size_t n) const {
		return m_frames[n];
	};

private:
	std::vector<sf::IntRect> m_frames;
	const sf::Texture* m_texture;
};

class AnimatedSprite : public sf::Drawable, public sf::Transformable
{
public:
	explicit AnimatedSprite(sf::Time frameTime = sf::seconds(0.2f), bool paused = false, bool looped = true);

	void update(sf::Time deltaTime);
	void setAnimation(const Animation& animation);
	void setFrameTime(sf::Time time);
	void play();
	void play(const Animation& animation);
	void pause();
	void stop();
	void setLooped(bool looped);
	void setColor(const sf::Color& color);
	const Animation* getAnimation() const;
	sf::FloatRect getLocalBounds() const;
	sf::FloatRect getGlobalBounds() const;
	bool isLooped() const;
	bool isPlaying() const;
	sf::Time getFrameTime() const;
	void setFrame(std::size_t newFrame, bool resetTime = true);

	sf::Time m_currentTime;
	std::size_t m_currentFrame;

private:
	const Animation* m_animation;
	sf::Time m_frameTime;
	
	
	bool m_isPaused;
	bool m_isLooped;
	const sf::Texture* m_texture;
	sf::Vertex m_vertices[4];

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

};

AnimatedSprite::AnimatedSprite(sf::Time frameTime, bool paused, bool looped) :
	m_animation(NULL), m_frameTime(frameTime), m_currentFrame(0), m_isPaused(paused), m_isLooped(looped), m_texture(NULL)
{

}

void AnimatedSprite::setAnimation(const Animation& animation)
{
	m_animation = &animation;
	m_texture = m_animation->getSpriteSheet();
	m_currentFrame = 0;
	setFrame(m_currentFrame);
}

void AnimatedSprite::setFrameTime(sf::Time time)
{
	m_frameTime = time;
}

void AnimatedSprite::play()
{
	m_isPaused = false;
}

void AnimatedSprite::play(const Animation& animation)
{
	if (getAnimation() != &animation)
		setAnimation(animation);
	play();
}

void AnimatedSprite::pause()
{
	m_isPaused = true;
}

void AnimatedSprite::stop()
{
	m_isPaused = true;
	m_currentFrame = 0;
	setFrame(m_currentFrame);
}

void AnimatedSprite::setLooped(bool looped)
{
	m_isLooped = looped;
}

void AnimatedSprite::setColor(const sf::Color& color)
{
	// Update the vertices' color
	m_vertices[0].color = color;
	m_vertices[1].color = color;
	m_vertices[2].color = color;
	m_vertices[3].color = color;
}

const Animation* AnimatedSprite::getAnimation() const
{
	return m_animation;
}

sf::FloatRect AnimatedSprite::getLocalBounds() const
{
	sf::IntRect rect = m_animation->getFrame(m_currentFrame);

	float width = static_cast<float>(std::abs(rect.width));
	float height = static_cast<float>(std::abs(rect.height));

	return sf::FloatRect(0.f, 0.f, width, height);
}

sf::FloatRect AnimatedSprite::getGlobalBounds() const
{
	return getTransform().transformRect(getLocalBounds());
}

bool AnimatedSprite::isLooped() const
{
	return m_isLooped;
}

bool AnimatedSprite::isPlaying() const
{
	return !m_isPaused;
}

sf::Time AnimatedSprite::getFrameTime() const
{
	return m_frameTime;
}

void AnimatedSprite::setFrame(std::size_t newFrame, bool resetTime)
{
	if (m_animation)
	{
		//calculate new vertex positions and texture coordiantes
		sf::IntRect rect = m_animation->getFrame(newFrame);

		m_vertices[0].position = sf::Vector2f(0.f, 0.f);
		m_vertices[1].position = sf::Vector2f(0.f, static_cast<float>(rect.height));
		m_vertices[2].position = sf::Vector2f(static_cast<float>(rect.width), static_cast<float>(rect.height));
		m_vertices[3].position = sf::Vector2f(static_cast<float>(rect.width), 0.f);

		float left = static_cast<float>(rect.left) + 0.0001f;
		float right = left + static_cast<float>(rect.width);
		float top = static_cast<float>(rect.top);
		float bottom = top + static_cast<float>(rect.height);

		m_vertices[0].texCoords = sf::Vector2f(left, top);
		m_vertices[1].texCoords = sf::Vector2f(left, bottom);
		m_vertices[2].texCoords = sf::Vector2f(right, bottom);
		m_vertices[3].texCoords = sf::Vector2f(right, top);
	}

	if (resetTime)
		m_currentTime = sf::Time::Zero;
}

void AnimatedSprite::update(sf::Time deltaTime)
{
	// if not paused and we have a valid animation
	if (!m_isPaused && m_animation)
	{
		// add delta time
		m_currentTime += deltaTime;

		// if current time is bigger then the frame time advance one frame
		if (m_currentTime >= m_frameTime)
		{
			// reset time, but keep the remainder
			m_currentTime = sf::microseconds(m_currentTime.asMicroseconds() % m_frameTime.asMicroseconds());

			// get next Frame index
			if (m_currentFrame + 1 < m_animation->getSize())
				m_currentFrame++;
			else
			{
				// animation has ended
				m_currentFrame = 0; // reset to start

				if (!m_isLooped)
				{
					m_isPaused = true;
				}

			}

			// set the current frame, not reseting the time
			setFrame(m_currentFrame, false);
		}
	}
}

void AnimatedSprite::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (m_animation && m_texture)
	{
		states.transform *= getTransform();
		states.texture = m_texture;
		target.draw(m_vertices, 4, sf::Quads, states);
	}
}