#pragma once
#include "Math.h"

#include "SDL/include/SDL.h"
#include "imgui-master/imgui.h"

#include <unordered_map>

extern bool g_running;

struct Key {
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};
extern std::unordered_map<int32, Key> keyStates;

struct Mouse {
    Vec2Int pos = {};
    Vec2 pDelta = {};
    Vec2Int wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
    SDL_Cursor* cursors[ImGuiMouseCursor_COUNT] = {};
    bool canUseGlobalState = true;
};
extern Mouse g_mouse;


//g_actions[ACTION_MAIN].held
//g_actions[ACTION_MAIN].pressed
//g_actions[ACTION_MAIN].realeased
