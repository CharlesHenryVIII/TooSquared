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

struct GameData {
    float m_currentTime = 12.0f;
    float m_timeScale = 1.0f;
}g_gameData;

//returns false if out of range
bool HeavensInterpolation(float& result, float time, float lo, float hi, float interpolate_lo, float interpolate_hi)
{
    result = 0;
    if (time > lo && time < hi)
    {
        if (time > interpolate_lo && time < interpolate_hi)
        {
            result = 1.0f;
        }
        else
        {
            result = 0.0f;
            if (time <= interpolate_lo)
            {
                //approaching 1.0 as sun comes up and time goes 6.0
                result = fabs((time - lo) / (interpolate_lo - lo));
            }
            else
            {
                //approaching 0.0 as sun goes down and time goes 18.0
                result = fabsf(1 - ((time - interpolate_hi) / (hi - interpolate_hi)));
            }
        }
    }
    else
    {
        result = 0;
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    std::unordered_map<int32, Key> keyStates;
    InitializeVideo();
    MultiThreading& multiThreading = MultiThreading::GetInstance();
    Camera testCamera;


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
    gb_mat4_look_at(&g_camera.view, { g_camera.p.p.x + cOffset.p.x, g_camera.p.p.y + cOffset.p.y,g_camera.p.p.z + cOffset.p.z }, { g_camera.p.p.x, g_camera.p.p.y, g_camera.p.p.z }, { 0,1,0 });

    g_chunks = new ChunkArray();
    float loadingTimer = 0.0f;
    bool uploadedLastFrame = false;
    bool debugDraw = false;
    bool TEST_CREATE_AND_UPLOAD_CHUNKS = true;

    //int32 area = 10;
    //for (int32 y = -area; y <= area; y++)
    //{
    //    for (int32 x = -area; x <= area; x++)
    //    {
    //        //Vec3 a = Voronoi_DAndP(Vec2({ float(x), float(y) }) / float(INT_MAX));
    //        Vec3 a = Voronoi_DAndP(Vec2({ float(x), float(y) }));
    //        DebugPrint("%i, %i : %0.3f, %0.3f, W: %0.3f \n", x, y, a.x, a.y, a.z);
    //    }
    //}

    //float area = 0.1f;
    //for (float y = 0; y <= area + FLT_EPSILON; y += 0.001f)
    //{
    //    for (float x = 0; x <= area + FLT_EPSILON; x += 0.001f)
    //    {
    //        //Vec3 a = Voronoi_DAndP(Vec2({ float(x), float(y) }) / float(INT_MAX));
    //        Vec3 a = Voronoi_DAndP(100 * Vec2({ x, y }));
    //        DebugPrint("%+0.6f, %+0.6f : %+0.6f, %+0.6f, W: %+0.6f \n", x, y, a.y, a.z, a.x);
    //    }
    //}


    //const float testSize = 10.0f;
    //for (float x = -testSize; x <= testSize; x += 0.1f)
    //{
    //    for (float y = -testSize; y <= testSize; y += 0.1f)
    //    {
    //        DebugPrint("VoronoiNoise at %+05.1f, %+05.1f: %+05.4f\n", x, y, VoronoiNoise({ x, y }, 1.0f, 0.0f));
    //    }
    //}

    cubesToDraw.reserve(100000);
    while (g_running)
    {
        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);// / 10;
        previousTime = totalTime;
        g_gameData.m_currentTime = fmod(g_gameData.m_currentTime + deltaTime * g_gameData.m_timeScale, 24.0f);

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
        {
            ChunkPos cameraChunk = ToChunk(g_camera.p);
            SDL_SetWindowTitle(g_window.SDL_Context, ToString("TooSquared P: %i, %i, %i; C: %i, %i; Chunks: %u; Time: %0.2f; Triangles: %u",
                (int32)floor(g_camera.p.p.x), (int32)floor(g_camera.p.p.y), (int32)floor(g_camera.p.p.z), cameraChunk.p.x, cameraChunk.p.z, g_chunks->chunkCount, loadingTimer, g_renderer.numTrianglesDrawn).c_str());
        }

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
            g_camera.p.p += (cameraSpeed * g_camera.front);
        if (keyStates[SDLK_s].down)
            g_camera.p.p -= (cameraSpeed * g_camera.front);
        if (keyStates[SDLK_a].down)
            g_camera.p.p -= (Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed);
        if (keyStates[SDLK_d].down)
            g_camera.p.p += (Normalize(Cross(g_camera.front, g_camera.up)) * cameraSpeed);
        if (keyStates[SDLK_LCTRL].down)
            g_camera.p.p.y -= cameraSpeed;
        if (keyStates[SDLK_SPACE].down)
            g_camera.p.p.y += cameraSpeed;
        if (keyStates[SDLK_z].down)
            g_camera.p.p.z += cameraSpeed;
        if (keyStates[SDLK_x].down)
            g_camera.p.p.x += cameraSpeed;
        if (keyStates[SDLK_c].downThisFrame)
            TEST_CREATE_AND_UPLOAD_CHUNKS = !TEST_CREATE_AND_UPLOAD_CHUNKS;
        if (keyStates[SDLK_m].downThisFrame)
        {
            switch (multiThreading.threads)
            {
            case MultiThreading::single_thread:
                multiThreading.threads = MultiThreading::multi_thread;
                break;
            case MultiThreading::multi_thread:
                multiThreading.threads = MultiThreading::single_thread;
                break;
            }
        }

        // change this value to your liking
        float sensitivity = 0.3f; 
        g_mouse.pDelta *= sensitivity;


        if (keyStates[SDL_BUTTON_LEFT].down)
        {
            g_camera.yaw += g_mouse.pDelta.x;
            g_camera.pitch -= g_mouse.pDelta.y;
        }

        if (keyStates[SDLK_t].downThisFrame)
        {
            if (keyStates[SDLK_LSHIFT].down)
            {
                g_gameData.m_timeScale /= 10;
            }
            else if (keyStates[SDLK_LCTRL].down)
            {
                g_gameData.m_currentTime = 10;
                g_gameData.m_timeScale = 0;
            }
            else
            {
                g_gameData.m_timeScale *= 10;
            }
        }


        //make sure that when pitch is out of bounds, screen doesn't get flipped
        g_camera.pitch = Clamp<float>(g_camera.pitch, -89.0f, 89.0f);

        Vec3 front = {};
        front.x = cos(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
        front.y = sin(DegToRad(g_camera.pitch));
        front.z = sin(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
        g_camera.front = Normalize(front);

        Vec3 lookTarget = g_camera.p.p + g_camera.front;
        gb_mat4_look_at(&g_camera.view, g_camera.p.p, lookTarget, g_camera.up);

        float SunRotationRadians = (((g_gameData.m_currentTime - 6.0f) / 24) * tau);
        float sunRotationCos = cosf(SunRotationRadians);
        g_renderer.sunLight.d = Normalize(Vec3({ -sunRotationCos, -sinf(SunRotationRadians),  0.0f }));
        g_renderer.moonLight.d = -g_renderer.sunLight.d;
        const Color sunTransitionColor  = { 220 / 255.0f,  90 / 255.0f,  40 / 255.0f, 1.0f};
        const Color moonTransitionColor = {  80 / 255.0f,  80 / 255.0f,  90 / 255.0f, 1.0f};

        //TODO: Fix this garbage shit:
        {

            float percentOfSun = 0;
            if (HeavensInterpolation(percentOfSun, g_gameData.m_currentTime, 5.9f, 18.1f, 6.1f, 17.9f))
            {
                g_renderer.sunLight.c.r = Lerp<float>(Lerp<float>(White.r, sunTransitionColor.r, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
                g_renderer.sunLight.c.g = Lerp<float>(Lerp<float>(White.g, sunTransitionColor.g, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
                g_renderer.sunLight.c.b = Lerp<float>(Lerp<float>(White.b, sunTransitionColor.b, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
            }
            else
                g_renderer.sunLight.c = { 0, 0, 0 };

            float percentOfMoon = 0;
            //percentOfMoon = 1 - percentOfSun;
            if (HeavensInterpolation(percentOfMoon, fmodf(g_gameData.m_currentTime + 12.0f, 24.0f), 5.9f, 18.1f, 6.1f, 17.9f))
            {
                g_renderer.moonLight.c.r = Lerp<float>(moonTransitionColor.r, {}, 1 - percentOfMoon);
                g_renderer.moonLight.c.g = Lerp<float>(moonTransitionColor.g, {}, 1 - percentOfMoon);
                g_renderer.moonLight.c.b = Lerp<float>(moonTransitionColor.b, {}, 1 - percentOfMoon);
            }
            else
                g_renderer.moonLight.c = { 0, 0, 0 };

        }
        //END OF GARBAGE?





        GamePos hitBlock;
        bool validHit = false;
        Vec3 hitNormal;

        {
            RegionSampler localRegion;
            ChunkIndex centerChunkIndex;
            Ray ray = {
                .origin = g_camera.p.p,
                .direction = lookTarget - g_camera.p.p,
            };
            if (g_chunks->GetChunkFromPosition(centerChunkIndex, ToChunk(g_camera.p)))
            {
                //bool RayVsChunk(const Ray & ray, ChunkIndex chunkIndex, GamePos & block, float& distance);
                GamePos resultPos = {};
                float distance;
                if (RayVsChunk(ray, centerChunkIndex, resultPos, distance, hitNormal))
                {
                    hitBlock = resultPos;
                    validHit = (distance < 5.0f);
                }

                if (!validHit && localRegion.RegionGather(centerChunkIndex))
                {
                    for (ChunkIndex neighbor : localRegion.neighbors)
                    {
                        float distanceComparison;
                        Vec3 neighborNormal;
                        if (RayVsChunk(ray, neighbor, resultPos, distanceComparison, neighborNormal))
                        {
                            if (distanceComparison < distance)
                            {
                                hitBlock = resultPos;
                                distance = distanceComparison;
                                hitNormal = neighborNormal;
                            }
                        }
                    }
                    validHit = (distance < 5.0f);
                }
            }
        }

        //TODO: Optimize to update corners (max 4 chunks)
        if (keyStates[SDL_BUTTON_RIGHT].downThisFrame)
        {
            if (validHit)
            {
                SetBlock(hitBlock, {}, BlockType::Empty);
            }
        }
        //TODO: Optimize to update corners (max 4 chunks)
        for (int32 c = SDLK_1; c <= SDLK_9; c++)
        {
            //if (keyStates[c].downThisFrame)
            if (keyStates[c].down)
            {
                if (validHit)
                {
                    SetBlock(hitBlock, hitNormal, BlockType(c - SDLK_1 + 1));
                    break;
                }
            }
        }

        //testCamera.p = { 0, 100, 0 };
        //gb_mat4_look_at(&g_camera.view, g_camera.p.p, testCamera.p.p, {0, 1, 0});

        //Vec3 lookatPosition = { (float)sin(totalTime / 10) * 100, 100, (float)cos(totalTime / 10) * 100};
        //gb_mat4_look_at(&testCamera.view, testCamera.p.p, lookatPosition, { 0, 1, 0 });

        Mat4 perspective;
        gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.2f, 2000.0f);
        Mat4 viewProj = perspective * g_camera.view;//testCamera.view;



        {
            //PROFILE_SCOPE("Camera Position Chunk Update");

#ifdef _DEBUG
            g_camera.drawDistance = 10;
#elif NDEBUG
            g_camera.drawDistance = 20;//60;
#endif

            g_camera.fogDistance = g_camera.drawDistance + 10;
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
                            ChunkPos newBlockP = { cam.p.x + x, 0, cam.p.z + z };
                            ChunkIndex funcResult;
                            if (TEST_CREATE_AND_UPLOAD_CHUNKS)
                            if (!g_chunks->GetChunkFromPosition(funcResult, newBlockP))
                            {
                                ChunkIndex chunki = g_chunks->AddChunk(newBlockP);
                            }
                        }
                    }
                }
            }

            {
                //PROFILE_SCOPE("Chunk Delete Check");
                if (g_camera.fogDistance)
                {
                    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
                    {
                        if (g_chunks->active[i])
                        {
                            if (((g_chunks->p[i].p.x > cam.p.x + g_camera.fogDistance || g_chunks->p[i].p.z > cam.p.z + g_camera.fogDistance) ||
                                (g_chunks->p[i].p.x < cam.p.x - g_camera.fogDistance || g_chunks->p[i].p.z < cam.p.z - g_camera.fogDistance)) ||
                                !TEST_CREATE_AND_UPLOAD_CHUNKS)
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


        //SKYBOX
        {
            //glBlendFunc(GL_ONE, GL_ONE);

            glDepthMask(GL_FALSE);
            ShaderProgram* sp = g_renderer.programs[+Shader::Sun];
            sp->UseShader();
            sp->UpdateUniformVec3("u_directionalLight_d", 1, g_renderer.sunLight.d.e);
            sp->UpdateUniformVec3("u_sunColor", 1, g_renderer.sunLight.c.e);
            sp->UpdateUniformVec3("u_directionalLightMoon_d", 1, g_renderer.moonLight.d.e);
            sp->UpdateUniformVec3("u_moonColor", 1, g_renderer.moonLight.c.e);
            Mat4 iViewProj;
            gb_mat4_inverse(&iViewProj, &viewProj);
            sp->UpdateUniformMat4("u_inverseViewProjection", 1, false, iViewProj.e);
            sp->UpdateUniformVec3("u_cameraPosition", 1, g_camera.p.p.e);
            sp->UpdateUniformFloat("u_gameTime", g_gameData.m_currentTime);
            glActiveTexture(GL_TEXTURE1);
            g_renderer.skyBoxNight->Bind();
            glActiveTexture(GL_TEXTURE0);
            g_renderer.skyBoxDay->Bind();

            g_renderer.backBuffer->m_vertexBuffer.Bind();

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
            glEnableVertexArrayAttrib(g_renderer.vao, 0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glEnableVertexArrayAttrib(g_renderer.vao, 1);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
            glEnableVertexArrayAttrib(g_renderer.vao, 2);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glDepthMask(GL_TRUE);
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

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
                        ChunkPos drawDistanceChunk = { cameraChunkP.p.x + drawX, 0, cameraChunkP.p.z + drawZ };
                        if (!g_chunks->GetChunkFromPosition(originChunk, drawDistanceChunk))
                            continue;
                        if (g_chunks->state[originChunk] != ChunkArray::BlocksLoaded)
                            continue;

                        RegionSampler regionSampler = {};

                        if (regionSampler.RegionGather(originChunk))
                        {
                            CreateVertices* job = new CreateVertices();
                            job->region = regionSampler;

                            g_chunks->state[originChunk] = ChunkArray::VertexLoading;
                            multiThreading.SubmitJob(job);
                        }
                    }
                }
            }
        }

        {
            //PROFILE_SCOPE("Chunk Upload and Render");

#ifdef _DEBUG
            const int32 uploadMax = 10;
#elif NDEBUG
            //const int32 uploadMax = 40;
            const int32 uploadMax = 300;
#endif
            struct Renderable {
                ChunkIndex r;
                int32 d;
            };
            Renderable renderables[MAX_CHUNKS];
            int32 numRenderables = 0;
            Frustum frustum = ComputeFrustum(viewProj);
            int32 uploadCount = 0;
            PreChunkRender(perspective);
            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                if (!g_chunks->active[i])
                    continue;

                ChunkPos chunkP = g_chunks->p[i];
                GamePos min = ToGame(chunkP);
                GamePos max = { min.p.x + (int32)CHUNK_X, min.p.y + (int32)CHUNK_Y, min.p.z + (int32)CHUNK_Z };
                if (IsBoxInFrustum(frustum, ToWorld(min).p.e, ToWorld(max).p.e))
                {
                    renderables[numRenderables].d = ManhattanDistance(ToChunk(g_camera.p.p).p, chunkP.p);
                    renderables[numRenderables++].r = i;
                }
            }

            std::sort(std::begin(renderables), std::begin(renderables) + numRenderables, [](Renderable a, Renderable b) {
                return a.d < b.d;
            });
            for (int32 i = 0; i < numRenderables; i++)
            {
                ChunkIndex renderChunk = renderables[i].r;
                if (g_chunks->state[renderChunk] == ChunkArray::VertexLoaded)
                {
                    if (uploadCount > uploadMax || (g_chunks->flags[renderChunk] & CHUNK_TODELETE))
                        continue;
                    g_chunks->UploadChunk(renderChunk);
                    uploadCount++;
                    uploadedLastFrame = true;
                }
                if (g_chunks->state[renderChunk] == ChunkArray::Uploaded)
                {
                    g_chunks->RenderChunk(renderChunk);
                }
            }
        }

        {
            //DrawBlock(testCamera.p.p, { 0, 1, 0, 1 }, 5.0f, perspective);
            //DrawBlock(lookatPosition, { 1, 0, 0, 1 }, 5.0f, perspective);
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
                    chunkP.p.x += CHUNK_X / 2.0f;
                    //chunkP.p.y = CHUNK_Y;
                    chunkP.p.z += CHUNK_Z / 2.0f;
                    Vec3 size = { CHUNK_X / 4.0f, CHUNK_Y, CHUNK_Z / 4.0f };

                    DrawBlock(chunkP, colors[static_cast<int32>(g_chunks->state[i])], size, perspective);
                }
                for (WorldPos p : cubesToDraw)
                {
                    DrawBlock(p, Red, 2.0f, perspective);
                }
            }
            if (validHit)
            {
                WorldPos pos;
                pos = ToWorld(hitBlock);
                pos.p = pos.p + 0.5f;
                Color temp = Mint;
                temp.a = 0.6f;
                DrawBlock(pos, temp, 1.1f, perspective);
            }
        }

        {
            //PROFILE_SCOPE("Chunk Deletion");
            for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
            {
                assert(g_chunks->refs[i] >= 0);
                if (!g_chunks->active[i])
                    continue;
                if (g_chunks->state[i] == ChunkArray::VertexLoading)
                    continue;
                if (g_chunks->state[i] == ChunkArray::BlocksLoading)
                    continue;

                if ((g_chunks->flags[i] & CHUNK_TODELETE) && (g_chunks->refs[i] == 0))
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
