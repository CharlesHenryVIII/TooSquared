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
#include "Computer.h"
#include "WinInterop.h"

#include <unordered_map>
#include <vector>

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

    g_jobHandler.semaphore = SDL_CreateSemaphore(0);
	g_jobHandler.mutex = SDL_CreateMutex();
    g_jobHandler.wait_semaphore = SDL_CreateSemaphore(0);
    g_computerSpecs.coreCount = SDL_GetCPUCount();
	g_jobHandler.threads.resize(g_computerSpecs.UsableCores());

    ThreadData passingData;
    SDL_AtomicSet(&passingData.running, 1);
    for (uint32 i = 0; i < g_jobHandler.threads.size(); ++i)
    {
        g_jobHandler.threads[i] = SDL_CreateThread(ThreadFunction, ("Thread " + std::to_string(i)).c_str(), &passingData);
		DebugPrint("Created New Thread: %i\n", i);
    }

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	srand(static_cast<uint32>(totalTime));
	double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    g_camera.view;
	Vec3 cOffset = { 1.0f, 1.0f, 1.0f };
	gb_mat4_look_at(&g_camera.view, g_camera.p + cOffset, g_camera.p, { 0,1,0 });

	std::vector<Chunk*> chunks;
	std::vector<Chunk*> chunksToLoad;
	std::vector<double> values;
	//values.reserve(size_t(2 * (2 / 0.01)));
#ifdef IMPLIMENTATION4
	PerlinInit();
#endif

	//{
	//float max = -20.0f;
	//float min =  20.0f;
	//double totalValue = 0;
	//uint64 totalNumber = 0;

	//	//for (double z = 0; z <= 1.0f; z += 0.1f)
	//	//{
	//		for (float y = 0.1f; y <= 20; y += 0.1f)
	//		{
	//			for (float x = 0.1f; x <= 20; x += 0.1f)
	//			{
	//				//double a = Perlin(x, y, z);
	//				//values.push_back(a);
	//				float a = Terrain({ x, y });
	//				totalValue += a;
	//				totalNumber++;
	//				max = Max(a, max);
	//				min = Min(a, min);
	//				DebugPrint("TER{ %.03f, %.03f } = %f\n", x, y, a);
	//			}
	//		}

	//	//}
	//DebugPrint("Total Number of iterations: %i\n", totalNumber);
	//DebugPrint("Total Value: %f\n", totalValue);
	//DebugPrint("Min Value: %f\n", min);
	//DebugPrint("Max Value: %f\n", max);
	//}

	//{
	//float max = -20.0f;
	//float min =  20.0f;
	//double totalValue = 0;
	//uint64 totalNumber = 0;

	//	//for (double z = 0; z <= 1.0f; z += 0.1f)
	//	//{
	//		for (float y = -20; y <= 20; y += 1)
	//		{
	//			for (float x = -20; x <= 20; x += 1)
	//			{
	//				//double a = Perlin(x, y, z);
	//				//values.push_back(a);
	//				float a = FBM({ x, y }, 0.5f);
	//				totalValue += a;
	//				totalNumber++;
	//				max = Max(a, max);
	//				min = Min(a, min);
	//				DebugPrint("FBM{ %.03f, %.03f } = %f\n", x, y, a);
	//			}
	//		}

	//	//}
	//DebugPrint("Total Number of iterations: %i\n", totalNumber);
	//DebugPrint("Total Value: %f\n", totalValue);
	//DebugPrint("Min Value: %f\n", min);
	//DebugPrint("Max Value: %f\n", max);
	//}

	//double test1 = Perlin(0, 0, 0);
	//double test2 = Perlin(1, 0, 0);
	//double test3 = Perlin(-0.12f, -0.12f, -0.12f);
	//double test4 = Perlin( 0.12f,  0.12f,  0.12f);

	//const int32 cubeSize = 10;
	//for (int32 z = -cubeSize; z <= cubeSize; z++)
	//{
	//	for (int32 x = -cubeSize; x <= cubeSize; x++)
	//	{
	//		Chunk* chunk = new Chunk();
	//		chunk->p = {x, 0, z};
	//		chunksToLoad.push_back(chunk);
	//	}
	//}

#if 0
	std::vector<Block*> blockList;
    {
		const int32 cubeSize = 5;
		for (float z = -cubeSize; z <= cubeSize; z++)
		{
			for (float y = -(2 * cubeSize); y <= 0; y++)
			{
				for (float x = -cubeSize; x <= cubeSize; x++)
				{
					Grass* temp = new Grass();
					temp->p = { x, y, z };
					blockList.push_back(temp);
				}
			}
		}
		{
			Grass* grass = new Grass();
			grass->p = { 0.0f, 1.0f, 0.0f };
			blockList.push_back(grass);

			Stone* stone = new Stone();
			stone->p = { 1.0f, 1.0f, 0.0f };
			blockList.push_back(stone);

			IronBlock* ironBlock = new IronBlock();
			ironBlock->p = { -1.0f, 1.0f, 0.0f };
			blockList.push_back(ironBlock);

			FireBlock* fireBlock = new FireBlock();
			fireBlock->p = { 10.0f, 10.0f, 10.0f };
			blockList.push_back(fireBlock);

		}
	}
#endif

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

		float cameraSpeed = 5.0f * deltaTime;
        if (keyStates[SDLK_LSHIFT].down)
            cameraSpeed *= 20;
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
		g_camera.pitch = Clamp<float>(g_camera.pitch, -89.0f, 89.0f);

		Vec3 front = {};
		front.x = cos(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
		front.y = sin(DegToRad(g_camera.pitch));
		front.z = sin(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
		g_camera.front = Normalize(front);

		gb_mat4_look_at(&g_camera.view, g_camera.p, g_camera.p + g_camera.front, g_camera.up);

		{
			//PROFILE_SCOPE("Camera Position Chunk Update");

			const int32 drawDistance = 10;
			const int32 fogDistance = 0;
			Vec3Int cam = ToChunkPosition(g_camera.p);
			for (int32 z = -drawDistance; z <= drawDistance; z++)
			{
				for (int32 x = -drawDistance; x <= drawDistance; x++)
				{
					bool needCube = true;
					Vec3Int newBlockP = { cam.x + x, 0, cam.z + z };
					for (Chunk* chunk : chunks)
					{
						if (chunk)
						{
							if (chunk->p.z == newBlockP.z && chunk->p.x == newBlockP.x)
							{
								if (chunk->flags & (CHUNK_LOADED | CHUNK_LOADING | CHUNK_MODIFIED))
								{

									needCube = false;
									break;
								}
							}
							else if (fogDistance && (chunk->flags & CHUNK_LOADED) &&
								((chunk->p.x > cam.x + fogDistance || chunk->p.z > cam.z + fogDistance) ||
								 (chunk->p.x < cam.x - fogDistance || chunk->p.z < cam.z - fogDistance)))
							{

								chunk->flags |= CHUNK_TODELTE;
							}
						}
					}
					if (needCube)
					{
						Chunk* chunk = new Chunk;
						chunk->p = newBlockP;
						chunksToLoad.push_back(chunk);
					}
				}
			}
		}


		RenderUpdate(deltaTime);

		{
			//PROFILE_SCOPE("Semaphore Update");

			if (chunksToLoad.size())
			{
				SDL_LockMutex(g_jobHandler.mutex);
				//assert(g_jobHandler.jobs.empty());
				//const size_t coreCount = g_jobHandler.threads.size();
				for (Chunk* chunk : chunksToLoad)
				{
					if (chunk)
					{

						Job* job = new Job();
						job->chunk = chunk;
						chunks.push_back(chunk);
						g_jobHandler.jobs.push_back(job);
						SDL_SemPost(g_jobHandler.semaphore);
					}
				}
				chunksToLoad.clear();
				SDL_UnlockMutex(g_jobHandler.mutex);
				//SDL_SemWait(g_jobHandler.wait_semaphore);
			}
		}

		//for (Block* g : blockList)
		//{
		//    if (g)
		//        g->Render();
		//}
		{
			//PROFILE_SCOPE("Chunk Upload and Render");

			int32 uploadCount = 0;
			for (Chunk* chunk : chunks)
			{
				if (chunk)
				{
					if (chunk->flags & CHUNK_LOADED)
					{
						if (chunk->flags & CHUNK_NOTUPLOADED)
						{
							chunk->UploadChunk();
							uploadCount++;
						}
						chunk->RenderChunk();
					}
				}
				if (uploadCount > 10)
					break;
			}
		}

		std::erase_if(chunks, [](Chunk* chunk) {
			if (chunk->flags & CHUNK_TODELTE)
			{
				delete chunk;
				chunk = nullptr;
				return true;
			}
			return false;
		});

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
