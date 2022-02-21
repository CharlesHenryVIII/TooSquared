#include "Input.h"
#include "Rendering.h"
//#include "Rendering.h"
//#include "Rendering.h"
//
//#include "SDL/include/SDL.h"

//std::unordered_map<int32, Key> keyStates;
//Mouse g_mouse;

void CommandHandler::InputUpdate()
{
    for (auto& key : keyStates)
    {
        if (key.second.down)
        {
            key.second.upThisFrame = false;
            if (key.second.downPrevFrame)
            {
                //DebugPrint("KeyDown && DownPreviousFrame: %f\n", totalTime);
                key.second.downThisFrame = false;
            }
            else
            {
                //DebugPrint("Down this frame: %f\n", totalTime);
                key.second.downThisFrame = true;
            }
        }
        else
        {
            key.second.downThisFrame = false;
            if (key.second.downPrevFrame)
            {
                //DebugPrint("Up This frame: %f\n", totalTime);
                key.second.upThisFrame = true;
            }
            else
            {
                //DebugPrint("KeyNOTDown && NotDownPreviousFrame: %f\n", totalTime);
                key.second.upThisFrame = false;
            }
        }
        key.second.downPrevFrame = key.second.down;
    }

    if (mouse.wheelModifiedLastFrame)
    {
        mouse.wheelInstant.y = 0;
        mouse.wheelModifiedLastFrame = false;
    }
    else if (mouse.wheelInstant.y)
    {
        mouse.wheelModifiedLastFrame = true;
    }

    // change this value to your liking
    mouse.pDelta *= m_sensitivity;

}

