#pragma once
#include "Math.h"

#include "SDL/include/SDL.h"
#include "imgui.h"

#include <unordered_map>

extern bool g_running;

struct Key {
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};
//extern std::unordered_map<int32, Key> keyStates;

struct Mouse {
    Vec2Int pos = {};
    Vec2 pDelta = {};
    Vec2Int wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
    Vec2Int wheelInstant = {};
    bool wheelModifiedLastFrame = false;
    SDL_Cursor* cursors[ImGuiMouseCursor_COUNT] = {};
    float m_sensitivity = 0.4f;
    bool canUseGlobalState = true;
};
//extern Mouse g_mouse;

static uint32 s_currentID = 0;
struct CommandHandler {
    std::unordered_map<int32, Key> keyStates;
    Mouse mouse = {};
    uint32 ID = ++s_currentID;
};

//g_actions[ACTION_MAIN].held
//g_actions[ACTION_MAIN].pressed
//g_actions[ACTION_MAIN].realeased
