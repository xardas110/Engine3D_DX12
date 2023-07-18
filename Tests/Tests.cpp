#include "Tests.h"
#include <iostream>
#include <Components.h>
#include <entity.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <gtest/gtest.h>

//Include all the tests below
#include <TextureManagerTest.h>

Tests::Tests(const std::wstring& name, int width, int height, bool vSync)
	:Game(name, width, height, vSync)
{
    srand(time(nullptr));
}

Tests::~Tests()
{}

bool Tests::LoadContent()
{
	retCode = RUN_ALL_TESTS();		
    return true;
}

void Tests::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
}

void Tests::UnloadContent()
{}
