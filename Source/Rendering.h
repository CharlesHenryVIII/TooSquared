#pragma once
#include "SDL/include/SDL.h"
#include "Math.h"

enum class ts_MessageBox {
	Invalid,
	Error = SDL_MESSAGEBOX_ERROR,
	Warning = SDL_MESSAGEBOX_WARNING,
	Informative = SDL_MESSAGEBOX_INFORMATION,
	Count,
};

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);

