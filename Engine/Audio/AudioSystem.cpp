#include <EnginePCH.h>
#include <AudioSystem.h>
#include <Game.h>
#include <Components.h>

AudioSystem::AudioSystem(Game* game) : m_Game(game)
{

}

void AudioSystem::Update()
{
	auto view = m_Game->registry.view<AudioComponent>();
	for (auto [entity, audio] : view.each())
	{
		audio.volume = 5.f;
	}
}
