#pragma once
#include <TypesCompat.h>

struct HDR
{
private:
	friend class DeferredRenderer;

	HDR() = delete;

	static void UpdateGUI();

	static TonemapCB& GetTonemapCB();
};