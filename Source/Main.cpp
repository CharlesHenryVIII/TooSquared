#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define _DEBUGPRINT
#include "SDL/include/SDL.h"
#include "Math.h"
#include "glew.h"
#include "STB/stb_image.h"
#include "Misc.h"
#include "Rendering.h"
#include "Block.h"

#include <unordered_map>

struct Key {
	bool down;
	bool downPrevFrame;
	bool downThisFrame;
	bool upThisFrame;
};

struct Mouse {
	Vec2Int pos = {};
    Vec2 pDelta = {};
	Vec2Int wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
}g_mouse;

int main(int argc, char* argv[])
{
    std::unordered_map<int32, Key> keyStates;
    InitializeVideo();

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	srand(static_cast<uint32>(totalTime));
	double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    g_camera.view;
	Vec3 a = { 1.0f, 1.0f, 1.0f };
	gb_mat4_look_at(&g_camera.view, g_camera.p + a, g_camera.p, { 0,1,0 });

	std::vector<Chunk*> chunks;

	const int32 drawDistance = 1;
	for (int32 z = - drawDistance; z < drawDistance; z++)
	{
		for (int32 x = -drawDistance; x < drawDistance; x++)
		{
			Chunk* chunk = new Chunk;
			chunk->p.x = x;
			chunk->p.z = z;
			chunk->SetBlocks();
			chunk->BuildChunkVertices();
			chunk->UploadChunk();
			chunks.push_back(chunk);
		}
	}

//	std::vector<Block*> blockList;
//    {
//		const int32 cubeSize = 5;
//		for (float z = -cubeSize; z <= cubeSize; z++)
//		{
//			for (float y = -(2 * cubeSize); y <= 0; y++)
//			{
//				for (float x = -cubeSize; x <= cubeSize; x++)
//				{
//					Grass* temp = new Grass();
//					temp->p = { x, y, z };
//					blockList.push_back(temp);
//				}
//			}
//		}
//		{
//			Grass* grass = new Grass();
//			grass->p = { 0.0f, 1.0f, 0.0f };
//			blockList.push_back(grass);
//
//			Stone* stone = new Stone();
//			stone->p = { 1.0f, 1.0f, 0.0f };
//			blockList.push_back(stone);
//
//			IronBlock* ironBlock = new IronBlock();
//			ironBlock->p = { -1.0f, 1.0f, 0.0f };
//			blockList.push_back(ironBlock);
//
//			FireBlock* fireBlock = new FireBlock();
//			fireBlock->p = { 10.0f, 10.0f, 10.0f };
//			blockList.push_back(fireBlock);
//
//		}
//	}

	double testTimer = totalTime;

	while (g_running)
    {
        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);// / 10;
        previousTime = totalTime;

        /*********************
         *
         * Event Queing and handling
         *
         ********/
        Vec2Int originalMouseLocation = {};
        Vec2Int newMouseLocation = {};

		g_mouse.pDelta = { 0, 0 };

		SDL_Event SDLEvent;
		while (SDL_PollEvent(&SDLEvent))
		{
			switch (SDLEvent.type)
			{
			case SDL_QUIT:
				g_running = false;
				break;
			case SDL_KEYDOWN:
			{
				if (g_window.hasAttention)
				{
					int mods = 0;
					keyStates[SDLEvent.key.keysym.sym].down = true;
				}
				break;
			}
			case SDL_KEYUP:
				if (g_window.hasAttention)
				{
					keyStates[SDLEvent.key.keysym.sym].down = false;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (g_window.hasAttention)
				{
					keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
				}
				break;
			case SDL_MOUSEMOTION:
			{
				if (g_window.hasAttention)
				{
					g_mouse.pDelta.x = static_cast<float>(SDLEvent.motion.x) - g_mouse.pos.x;
					g_mouse.pos.x = SDLEvent.motion.x;
					g_mouse.pDelta.y = static_cast<float>(SDLEvent.motion.y) - g_mouse.pos.y;// reversed since y-coordinates go from bottom to top
					g_mouse.pos.y = SDLEvent.motion.y;
				}
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				if (g_window.hasAttention)
				{
					g_mouse.wheel.x = SDLEvent.wheel.x;
					g_mouse.wheel.y = SDLEvent.wheel.y;
				}
				break;
			}

			case SDL_WINDOWEVENT:
			{
				switch (SDLEvent.window.event)
				{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					g_window.size.x = SDLEvent.window.data1;
					g_window.size.y = SDLEvent.window.data2;
					glViewport(0, 0, g_window.size.x, g_window.size.y);
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					g_window.hasAttention = true;
                    g_mouse.pDelta = {};
					SDL_CaptureMouse(SDL_TRUE);
					break;
				}
				case SDL_WINDOWEVENT_LEAVE:
				{
					//g_window.hasAttention = false;
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					g_window.hasAttention = false;
					break;
				}
				}
				break;
			}
			}
		}

        /*********************
         *
         * Setting Key States
         *
         ********/

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

        if (keyStates[SDLK_BACKQUOTE].down)
            g_running = false;

#ifdef CAMERA


		float cameraSpeed = 3.0f * deltaTime;
        if (keyStates[SDLK_LSHIFT].down)
            cameraSpeed *= 10;
		if (keyStates[SDLK_w].down)
			g_camera.p += cameraSpeed * g_camera.front;
		if (keyStates[SDLK_s].down)
			g_camera.p -= cameraSpeed * g_camera.front;
		if (keyStates[SDLK_a].down)
			g_camera.p -= Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed;
		if (keyStates[SDLK_d].down)
			g_camera.p += Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed;
		if (keyStates[SDLK_LCTRL].down)
			g_camera.p.y -= cameraSpeed;
		if (keyStates[SDLK_SPACE].down)
			g_camera.p.y += cameraSpeed;

		float sensitivity = 0.3f; // change this value to your liking
		g_mouse.pDelta *= sensitivity;


		if (keyStates[SDL_BUTTON_LEFT].down)
		{
			g_camera.yaw += g_mouse.pDelta.x;
			g_camera.pitch -= g_mouse.pDelta.y;
		}

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		Clamp<float>(g_camera.pitch, -89.0f, 89.0f);

		Vec3 front = {};
		front.x = cos(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
		front.y = sin(DegToRad(g_camera.pitch));
		front.z = sin(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
		g_camera.front = Normalize(front);

		gb_mat4_look_at(&g_camera.view, g_camera.p, g_camera.p + g_camera.front, g_camera.up);

#else
        float cameraMovement = 0.5f;
        if (keyStates[SDLK_LSHIFT].down)
            cameraMovement = 4.0f;
		g_camera.r = {};
        Vec3 cameraDelta = {};
        if (keyStates[SDLK_w].down)
            cameraDelta.z += cameraMovement * deltaTime;
        if (keyStates[SDLK_s].down)
            cameraDelta.z -= cameraMovement * deltaTime;
        if (keyStates[SDLK_a].down)
            cameraDelta.x += cameraMovement * deltaTime;
        if (keyStates[SDLK_d].down)
            cameraDelta.x -= cameraMovement * deltaTime;
        if (keyStates[SDLK_LCTRL].down)
            cameraDelta.y += cameraMovement * deltaTime;
        if (keyStates[SDLK_SPACE].down)
            cameraDelta.y -= cameraMovement * deltaTime;
        if (keyStates[SDLK_q].down)
            g_camera.r.z -= 0.5f * deltaTime;
        if (keyStates[SDLK_e].down)
            g_camera.r.z += 0.5f * deltaTime;



        Mat4 xRot;
        gb_mat4_identity(&xRot);
        Mat4 yRot;
        gb_mat4_identity(&yRot);
        Mat4 zRot;
		gb_mat4_rotate(&zRot, { 0, 0, 1 }, g_camera.r.z); //mouse y movement spin about the x coordinate
        Mat4 pos;
        gb_mat4_identity(&pos);

        if (keyStates[SDL_BUTTON_LEFT].down)
        {
			g_camera.r.x += g_mouse.pDelta.x * 0.5f * deltaTime;
			g_camera.r.y += g_mouse.pDelta.y * 0.5f * deltaTime;
			gb_mat4_rotate(&xRot, { 0, 1, 0 }, g_camera.r.x); //mouse x movement spin about the Y coordinate
			gb_mat4_rotate(&yRot, { 1, 0, 0 }, g_camera.r.y); //mouse y movement spin about the x coordinate
		}


		gb_mat4_translate(&pos, cameraDelta);
		g_camera.view = xRot * yRot * zRot * pos * g_camera.view;
#endif

        RenderUpdate(deltaTime);
        //for (Block* g : blockList)
        //{
        //    if (g)
        //        g->Render();
        //}
		for (Chunk* chunk : chunks)
		{
			chunk->RenderChunk();
		}

		//gb_mat4_look_at(&g_camera.view, g_camera.p + a, g_camera.p, { 0,1,0 });

        //double renderTotalTime = SDL_GetPerformanceCounter() / freq;
        //std::erase_if(frameTimes, [renderTotalTime](const float& a)
        //{
        //    return (static_cast<float>(renderTotalTime) - a> 1.0f);
        //});
        //frameTimes.push_back(static_cast<float>(renderTotalTime));
        SDL_GL_SwapWindow(g_window.SDL_Context);
    }
    return 0;
}
