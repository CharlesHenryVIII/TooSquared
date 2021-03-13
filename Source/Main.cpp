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
#include "Noise.h"

#include <unordered_map>
#include <vector>
#include <algorithm>

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
    MultiThreading& multiThreading = MultiThreading::GetInstance();

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;
#if 0
    srand(static_cast<uint32>(totalTime));
#else
    srand(14);
#endif
    NoiseInit();
    SetBlockSprites();

    g_camera.view;
    WorldPos cOffset = { 1.0f, 1.0f, 1.0f };
    gb_mat4_look_at(&g_camera.view, { g_camera.p.x + cOffset.x, g_camera.p.y + cOffset.y,g_camera.p.z + cOffset.z }, { g_camera.p.x, g_camera.p.y, g_camera.p.z }, { 0,1,0 });

    g_chunks = new ChunkArray();
    float loadingTimer = 0.0f;
    bool uploadedLastFrame = false;
    bool debugDraw = true;


    //const float testSize = 10.0f;
    //for (float x = -testSize; x <= testSize; x += 0.1f)
    //{
    //    for (float y = -testSize; y <= testSize; y += 0.1f)
    //    {
    //        DebugPrint("VoronoiNoise at %+05.1f, %+05.1f: %+05.4f\n", x, y, VoronoiNoise({ x, y }, 1.0f, 0.0f));
    //    }
    //}

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

        if (multiThreading.GetJobsInFlight() > 0 || uploadedLastFrame)
            loadingTimer += deltaTime;
        uploadedLastFrame = false;
        SDL_SetWindowTitle(g_window.SDL_Context, ToString("TooSquared Chunks: %u, Time: %0.2f, Triangles: %u", g_chunks->chunkCount, loadingTimer, g_renderer.numTrianglesDrawn).c_str());

        SDL_Event SDLEvent;
        g_mouse.pDelta = {};
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
                    g_mouse.pDelta.x += (static_cast<float>(SDLEvent.motion.x) - g_mouse.pos.x);
                    g_mouse.pos.x = SDLEvent.motion.x;
                    g_mouse.pDelta.y += (static_cast<float>(SDLEvent.motion.y) - g_mouse.pos.y);// reversed since y-coordinates go from bottom to top
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
                    SDL_GetMouseState(&g_mouse.pos.x, &g_mouse.pos.y);
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


        if (keyStates[SDLK_ESCAPE].down)
            g_running = false;
        if (keyStates[SDLK_BACKQUOTE].downThisFrame)
            debugDraw = !debugDraw;

        float cameraSpeed = 10.0f * deltaTime;
        if (keyStates[SDLK_LSHIFT].down)
            cameraSpeed *= 30;
        if (keyStates[SDLK_w].down)
            g_camera.p += (cameraSpeed * g_camera.front);
        if (keyStates[SDLK_s].down)
            g_camera.p -= (cameraSpeed * g_camera.front);
        if (keyStates[SDLK_a].down)
            g_camera.p -= (Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed);
        if (keyStates[SDLK_d].down)
            g_camera.p += (Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed);
        if (keyStates[SDLK_LCTRL].down)
            g_camera.p.y -= cameraSpeed;
        if (keyStates[SDLK_SPACE].down)
            g_camera.p.y += cameraSpeed;
        if (keyStates[SDLK_z].down)
            g_camera.p.z += cameraSpeed;
        if (keyStates[SDLK_x].down)
            g_camera.p.x += cameraSpeed;

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

        gb_mat4_look_at(&g_camera.view, { g_camera.p.x, g_camera.p.y, g_camera.p.z },
                       { g_camera.p.x + g_camera.front.x, g_camera.p.y + g_camera.front.y, g_camera.p.z + g_camera.front.z },
                         g_camera.up);

        {
            //PROFILE_SCOPE("Camera Position Chunk Update");

#ifdef _DEBUG
            g_camera.drawDistance = 10;
#elif NDEBUG
            g_camera.drawDistance = 40;
#endif

#if 0
            g_camera.fogDistance = 40;
            ChunkPos cam = ToChunk(g_camera.p);
            for (int32 z = -g_camera.drawDistance; z <= g_camera.drawDistance; z++)
            {
                for (int32 x = -g_camera.drawDistance; x <= g_camera.drawDistance; x++)
                {
                    ChunkPos newBlockP = { cam.x + x, 0, cam.z + z };
                    ChunkIndex funcResult;
                    if (!g_chunks->GetChunkFromPosition(funcResult, newBlockP))
                    {
                        ChunkIndex chunki = g_chunks->AddChunk(newBlockP);
                    }
                }
            }
#else
            g_camera.fogDistance = 40;
            ChunkPos cam = ToChunk(g_camera.p);
            for (int32 _drawDistance = 0; _drawDistance < g_camera.drawDistance; _drawDistance++)
            {
                for (int32 z = -_drawDistance; z <= _drawDistance; z++)
                {
                    for (int32 x = -_drawDistance; x <= _drawDistance; x++)
                    {
                        if (z == _drawDistance || x ==  _drawDistance ||
                           z == -_drawDistance || x == -_drawDistance)
                        {
                            ChunkPos newBlockP = { cam.x + x, 0, cam.z + z };
                            ChunkIndex funcResult;
                            if (!g_chunks->GetChunkFromPosition(funcResult, newBlockP))
                            {
                                ChunkIndex chunki = g_chunks->AddChunk(newBlockP);
                            }
                        }
                    }
                }
            }
#endif

            {
                //PROFILE_SCOPE("Chunk Delete Check");
                if (g_camera.fogDistance)
                {
                    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
                    {
                        if (g_chunks->active[i])
                        {
                            if ((g_chunks->p[i].x > cam.x + g_camera.fogDistance || g_chunks->p[i].z > cam.z + g_camera.fogDistance) ||
                                (g_chunks->p[i].x < cam.x - g_camera.fogDistance || g_chunks->p[i].z < cam.z - g_camera.fogDistance))
                            {
                                g_chunks->flags[i] |= CHUNK_TODELETE;
                            }
                        }
                    }
                }
            }
        }
        Vec2Int windowSizeThing = { g_window.size.x * 2, g_window.size.y * 2 };
        UpdateFrameBuffer(windowSizeThing);
        RenderUpdate(deltaTime);
        g_renderer.backBuffer->Bind();
        glViewport(0, 0, windowSizeThing.x, windowSizeThing.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            //PROFILE_SCOPE("Semaphore Update");

            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                if (!g_chunks->active[i])
                    continue;

                if (g_chunks->state[i] == ChunkArray::Unloaded)
                {
                    SetBlocks* job = new SetBlocks();
                    job->chunk = i;
                    g_chunks->state[i] = ChunkArray::BlocksLoading;
                    multiThreading.SubmitJob(job);
                }
            }
            //SDL_SemWait(g_jobHandler.wait_semaphore);
        }

        {
            //PROFILE_SCOPE("Chunk Loading Vertex Loop");

            //TODO::Make this two for loops relative to draw distance
            //and make it so it focuses on chunks closest to camera
            //like we are doing on the block generation stage
#if 1
            //slower but starts from the center
            for (int32 _drawDistance = 0; _drawDistance < g_camera.drawDistance; _drawDistance++)
            {
                for (int32 drawZ = -_drawDistance; drawZ <= _drawDistance; drawZ++)
                {
                    for (int32 drawX = -_drawDistance; drawX <= _drawDistance; drawX++)
                    {
                        if ((drawX < _drawDistance && drawX > -_drawDistance) &&
                            (drawZ < _drawDistance && drawZ > -_drawDistance))
                            continue;

                        ChunkPos cameraChunkP = ToChunk(g_camera.p);
                        ChunkIndex originChunk = 0;
                        ChunkPos drawDistanceChunk = { cameraChunkP.x + drawX, 0, cameraChunkP.z + drawZ };
                        if (!g_chunks->GetChunkFromPosition(originChunk, drawDistanceChunk))
                            continue;
                        if (g_chunks->state[originChunk] != ChunkArray::BlocksLoaded)
                            continue;

                        ChunkIndex indices[8] = {};
                        uint32 numIndices = 0;
                        for (int32 z = -1; z <= 1; z++)
                        {
                            for (int32 x = -1; x <= 1; x++)
                            {
                                if (x == 0 && z == 0)
                                    continue;
                                ChunkIndex chunkIndex = 0;
                                ChunkPos newBlockP = { cameraChunkP.x + drawX + x, 0, cameraChunkP.z + drawZ + z };
                                if (g_chunks->GetChunkFromPosition(chunkIndex, newBlockP))
                                {
                                    if (g_chunks->state[chunkIndex] >= ChunkArray::BlocksLoaded)
                                    {
                                        indices[numIndices++] = chunkIndex;
                                    }
                                }
                            }
                        }

                        if (numIndices == arrsize(indices))
                        {
                            CreateVertices* job = new CreateVertices();
                            static_assert(sizeof(job->neighbors) == sizeof(indices));

                            job->chunk = originChunk;
                            memcpy(job->neighbors, indices, sizeof(indices));
                            g_chunks->state[originChunk] = ChunkArray::VertexLoading;
                            multiThreading.SubmitJob(job);
                        }
                    }
                }
            }

#else
            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                if (!g_chunks->active[i])
                    continue;

                if (g_chunks->state[i] == ChunkArray::BlocksLoaded)
                {
                    GamePos p = Convert_ChunkIndexToGame(i);
                    ChunkIndex indices[8] = {};

                    uint32 numIndices = 0;
                    for (int32 x = -1; x <= 1; x++)
                    {
                        for (int32 z = -1; z <= 1; z++)
                        {
                            if (x == 0 && z == 0)
                                continue;
                            ChunkIndex chunkIndex = 0;
                            if (g_chunks->GetChunk(chunkIndex, { p.x + x * int32(CHUNK_X), p.y, p.z + z * int32(CHUNK_Z) }))
                            {
                                if (g_chunks->state[chunkIndex] >= ChunkArray::BlocksLoaded)
                                {
                                    indices[numIndices++] = chunkIndex;
                                }
                            }
                        }
                    }
                    if (numIndices == arrsize(indices))
                    {
                        CreateVertices* job = new CreateVertices();
                        static_assert(sizeof(job->neighbors) == sizeof(indices));

                        job->chunk = i;
                        memcpy(job->neighbors, indices, sizeof(indices));
                        g_chunks->state[i] = ChunkArray::VertexLoading;
                        multiThreading.SubmitJob(job);
                    }
                }
            }
#endif
        }

        {
            //PROFILE_SCOPE("Chunk Upload and Render");

#ifdef _DEBUG
            const int32 uploadMax = 10;
#elif NDEBUG
            const int32 uploadMax = 20;
#endif
#if 1
            int32 uploadCount = 0;
            PreChunkRender();
            for (int32 _drawDistance = 0; _drawDistance < g_camera.drawDistance; _drawDistance++)
            {
                for (int32 drawZ = -_drawDistance; drawZ <= _drawDistance; drawZ++)
                {
                    for (int32 drawX = -_drawDistance; drawX <= _drawDistance; drawX++)
                    {
                        if ((drawX < _drawDistance && drawX > -_drawDistance) &&
                            (drawZ < _drawDistance && drawZ > -_drawDistance))
                            continue;

                        ChunkPos cameraChunkP = ToChunk(g_camera.p);
                        ChunkIndex chunkIndex = 0;
                        ChunkPos drawDistanceChunk = { cameraChunkP.x + drawX, 0, cameraChunkP.z + drawZ };
                        if (!g_chunks->GetChunkFromPosition(chunkIndex, drawDistanceChunk))
                            continue;

                        if (!g_chunks->active[chunkIndex])
                            continue;

                        if (g_chunks->state[chunkIndex] == ChunkArray::VertexLoaded)
                        {
                            if (uploadCount > uploadMax)
                                continue;
                            g_chunks->UploadChunk(chunkIndex);
                            uploadCount++;
                            uploadedLastFrame = true;
                        }
                        if (g_chunks->state[chunkIndex] == ChunkArray::Uploaded)
                        {
                            g_chunks->RenderChunk(chunkIndex);
                        }
                    }
                }
            }

#else
            int32 uploadCount = 0;
            PreChunkRender();
            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                if (!g_chunks->active[i])
                    continue;

                if (g_chunks->state[i] == ChunkArray::VertexLoaded)
                {
                    if (uploadCount > uploadMax)
                        continue;
                    g_chunks->UploadChunk(i);
                    uploadCount++;
                    uploadedLastFrame = true;
                }
                if (g_chunks->state[i] == ChunkArray::Uploaded)
                {
                    g_chunks->RenderChunk(i);
                }
            }
#endif
        }

        {
            if (debugDraw)
            {
                for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
                {
                    if (!g_chunks->active[i])
                        continue;

                    Color colors[] = {
                        { 1, 0, 0, 0.4f },//Red    //Unloaded,
                        { 0, 1, 0, 0.4f },//Green  //BlocksLoading,
                        { 0, 0, 1, 0.4f },//Blue   //BlocksLoaded,
                        { 1, 1, 0, 0.4f },//Yellow //VertexLoading,
                        { 1, 0, 1, 0.4f },//Purple //VertexLoaded,
                        { 1, 1, 1, 0.4f },//White  //Uploaded,
                    };

                    WorldPos chunkP = ToWorld(Convert_ChunkIndexToGame(i));
                    chunkP.x += CHUNK_X / 2.0f;
                    //chunkP.y = CHUNK_Y;
                    chunkP.z += CHUNK_Z / 2.0f;
                    Vec3 size = { CHUNK_X, CHUNK_Y, CHUNK_Z };

                    DrawBlock(chunkP, colors[static_cast<int32>(g_chunks->state[i])], size);
                }
            }
        }

        {
            //PROFILE_SCOPE("Chunk Deletion");
            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                if (!g_chunks->active[i])
                    continue;
                if (g_chunks->state[i] == ChunkArray::VertexLoading)
                    continue;
                if (g_chunks->state[i] == ChunkArray::BlocksLoading)
                    continue;

                if (g_chunks->flags[i] & CHUNK_TODELETE)
                {
                    g_chunks->ClearChunk(i);
                }
            }
        }
        //gb_mat4_look_at(&g_camera.view, g_camera.p + a, g_camera.p, { 0,1,0 });

        //double renderTotalTime = SDL_GetPerformanceCounter() / freq;
        //std::erase_if(frameTimes, [renderTotalTime](const float& a)
        //{
        //    return (static_cast<float>(renderTotalTime) - a> 1.0f);
        //});
        //frameTimes.push_back(static_cast<float>(renderTotalTime));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, g_window.size.x, g_window.size.y);
        glBindTexture(GL_TEXTURE_2D, g_renderer.backBuffer->m_color->m_handle);
        g_renderer.programs[+Shader::BufferCopy]->UseShader();
        g_renderer.backBuffer->m_vertexBuffer.Bind();

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
        glEnableVertexArrayAttrib(g_renderer.vao, 2);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        SDL_GL_SwapWindow(g_window.SDL_Context);
        glEnable(GL_DEPTH_TEST);
    }
    return 0;
}
