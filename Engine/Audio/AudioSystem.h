#pragma once

struct Audio 
{
	float volume = 0.f;

};

class Game;

class AudioSystem
{
	friend class Game;
	AudioSystem(class Game* game);
	Game* m_Game = nullptr;

	void Update();
};