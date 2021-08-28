#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define _DEBUGPRINT
#include "SDL.h"
#include "Math.h"
#include "glew.h"
#include "STB/stb_image.h"
#include "Misc.h"
#include "Rendering.h"
#include "Chunk.h"
#include "Computer.h"
#include "WinInterop.h"
#include "Noise.h"
#include "Input.h"
#include "Gameplay.h"
#include "Entity.h"
#include "Raycast.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "tracy-master/Tracy.hpp"

#include <unordered_map>
#include <vector>
#include <algorithm>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

float s_timesOfDay[+TimeOfDay::Count] = { 0.0f, 7.0f, 10.0f, 17.0f };
const char* s_timesOfDayNames[+TimeOfDay::Count] = { "Midnight", "Morning", "Afternoon", "Evening" };

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

enum class ChunkUpdateOrigin : int32 {
    Player,
    Camera,
};
ENUMOPS(ChunkUpdateOrigin);

enum class DebugOptions : uint32 {
    None = 0,
    ChunkStatus = BIT(0),
    CollisionTriangles = BIT(1),
    LookatBlock = BIT(2),
    OldRaycast = BIT(3),
    Enabled = BIT(14),
    All = ChunkStatus | CollisionTriangles | LookatBlock,
    Count,
};
ENUMOPS(DebugOptions);

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ExitApplication(Player* player, Camera* camera)
{
//    g_items.SaveAll();
//
    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
    {
        if (g_chunks->flags[i] & CHUNK_FLAG_ACTIVE)
            g_chunks->flags[i] |= CHUNK_FLAG_TODELETE;
    }
    if (player)
    {
        player->Save();
    }
    if (camera)
    {
        camera->Save();
    }
    g_running = false;
}

int main(int argc, char* argv[])
{
    InitializeVideo();
    InitializeWinInterop();
    EntityInit();
    MultiThreading& multiThreading = MultiThreading::GetInstance();

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double startTime = SDL_GetPerformanceCounter() / freq;
    double totalTime = SDL_GetPerformanceCounter() / freq - startTime; //sec
    double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;
#if 0
    srand(static_cast<uint32>(totalTime));
#else
    srand(14);
#endif
    NoiseInit();
    BlockInit();

    //Initilizers
    g_chunks = new ChunkArray();
    g_chunks->Init();
    CommandHandler playerInput;
    Player* player = g_entityList.New<Player>();
    player->m_inputID = playerInput.ID;
    Camera* playerCamera = g_entityList.New<Camera>();
    player->ChildCamera(playerCamera);
    if (!player->Load())
    {
        player->m_transform.m_p.p = { 0, 150, 0 };
    }

    ChunkUpdateOrigin chunkUpdateOrigin = ChunkUpdateOrigin::Player;

    {
        WorldPos cOffset = { 1.0f, 0.0f, 1.0f };
        playerCamera->GetWorldPosition();
        gb_mat4_look_at(&playerCamera->m_view, 
                        playerCamera->m_transform.m_p.p,//{ g_camera.transform.m_p.p.x + cOffset.p.x, g_camera.transform.m_p.p.y + cOffset.p.y,g_camera.transform.m_p.p.z + cOffset.p.z },
                        playerCamera->m_transform.m_p.p + cOffset.p/*{ g_camera.transform.m_p.p.x, g_camera.transform.m_p.p.y, g_camera.transform.m_p.p.z }*/, { 0,1,0 });
    }

    Transform debug_blockTransformParent = {};
    debug_blockTransformParent.m_p.p = { 10, 150, 0 };
    Transform debug_blockTransformChild = {};
    debug_blockTransformChild.m_p.p = { 1, 1, 0 };


    float loadingTimer = 0.0f;
    bool uploadedLastFrame = false;
    bool showIMGUI = true;
    //bool debugDraw = false;
    uint32 s_debugFlags = +DebugOptions::LookatBlock | +DebugOptions::Enabled;
    bool TEST_CREATE_AND_UPLOAD_CHUNKS = true;
    BlockType ItemToInventoryFromImGUIType = BlockType::Empty;
    uint32 ItemToInventoryFromImGUICount = 0;

    //___________
    //IMGUI SETUP
    //___________

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(g_window.SDL_Context, g_renderer.GL_Context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& imGuiIO = ImGui::GetIO();

    bool g_cursorEngaged = true;

    cubesToDraw.reserve(100000);
    while (g_running)
    {
        {
            ZoneScopedN("Frame Update:");
            totalTime = SDL_GetPerformanceCounter() / freq - startTime;
            float deltaTime = float(totalTime - previousTime);// / 10;
            previousTime = totalTime;
            //TODO: Time stepping for simulation
            if (deltaTime > (1.0f / 60.0f))
                deltaTime = (1.0f / 60.0f);
            if (g_gameData.m_gameTimePlaying)
                g_gameData.m_currentTime = fmod(g_gameData.m_currentTime + deltaTime * g_gameData.m_timeScale, 24.0f);

            /*********************
             *
             * Event Queing and handling
             *
             ********/
            playerInput.mouse.pDelta = { 0, 0 };

            if (multiThreading.GetJobsInFlight() > 0 || uploadedLastFrame)
                loadingTimer += deltaTime;
            uploadedLastFrame = false;
            {
                SDL_SetWindowTitle(g_window.SDL_Context, ToString("TooSquared Chunks: %u; Time: %0.2f; Triangles: %u; grounded: %i",
                    g_chunks->chunkCount,
                    loadingTimer, g_renderer.numTrianglesDrawn, player->m_rigidBody.m_isGrounded).c_str());
            }



            SDL_Event SDLEvent;
            playerInput.mouse.pDelta = {};
            {
                ZoneScopedN("Poll Events");
                while (SDL_PollEvent(&SDLEvent))
                {
                    ImGui_ImplSDL2_ProcessEvent(&SDLEvent);

                    switch (SDLEvent.type)
                    {
                    case SDL_QUIT:
                        ExitApplication(player, playerCamera);
                        break;
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        if (g_window.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureKeyboard)
                            {
                                playerInput.keyStates[SDLEvent.key.keysym.sym].down = (SDLEvent.type == SDL_KEYDOWN);
                            }
                        }
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        if (g_window.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                playerInput.keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                            }
                        }
                        break;
                    case SDL_MOUSEMOTION:
                    {
                        if (g_window.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                if (g_cursorEngaged)
                                {
                                    playerInput.mouse.pDelta.x += (static_cast<float>(SDLEvent.motion.x) - playerInput.mouse.pos.x);
                                    playerInput.mouse.pDelta.y += (static_cast<float>(SDLEvent.motion.y) - playerInput.mouse.pos.y);// reversed since y-coordinates go from bottom to top

                                    SDL_WarpMouseInWindow(g_window.SDL_Context, g_window.size.x / 2, g_window.size.y / 2);
                                    //playerInput.mouse.pos.x = SDLEvent.motion.x;
                                    //playerInput.mouse.pos.y = SDLEvent.motion.y;
                                    playerInput.mouse.pos.x = g_window.size.x / 2;
                                    playerInput.mouse.pos.y = g_window.size.y / 2;
                                }

                            }
                        }
                        break;
                    }
                    case SDL_MOUSEWHEEL:
                    {
                        if (g_window.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                playerInput.mouse.wheelInstant.x = playerInput.mouse.wheel.x = SDLEvent.wheel.x;
                                playerInput.mouse.wheelInstant.y = playerInput.mouse.wheel.y = SDLEvent.wheel.y;
                            }
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
                            playerInput.mouse.pDelta = {};
                            SDL_GetMouseState(&playerInput.mouse.pos.x, &playerInput.mouse.pos.y);
                            if (g_cursorEngaged)
                            {
                                SDL_CaptureMouse(SDL_TRUE);
                                SDL_ShowCursor(SDL_DISABLE);
                            }

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
            }

            /*********************
             *
             * Setting Key States
             *
             ********/

            {
                ZoneScopedN("Key Updates");
                for (auto& key : playerInput.keyStates)
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
                if (playerInput.mouse.wheelModifiedLastFrame)
                {
                    playerInput.mouse.wheelInstant.y = 0;
                    playerInput.mouse.wheelModifiedLastFrame = false;
                }
                else if (playerInput.mouse.wheelInstant.y)
                {
                    playerInput.mouse.wheelModifiedLastFrame = true;
                }


                if (playerInput.keyStates[SDLK_0].downThisFrame)
                {
                    if (player->m_hasCamera)
                    {
                        player->DecoupleCamera();
                    }
                    else
                    {
                        player->ChildCamera(playerCamera);
                    }
                }



                if (playerInput.keyStates[SDLK_ESCAPE].down)
                    ExitApplication(player, playerCamera);
                if (playerInput.keyStates[SDLK_BACKQUOTE].downThisFrame)
                    s_debugFlags ^= +DebugOptions::Enabled;
                //if (playerInput.keyStates[SDLK_c].downThisFrame)
                //    TEST_CREATE_AND_UPLOAD_CHUNKS = !TEST_CREATE_AND_UPLOAD_CHUNKS;
                if (playerInput.keyStates[SDLK_v].downThisFrame)
                    g_renderer.msaaEnabled = !g_renderer.msaaEnabled;
                if (playerInput.keyStates[SDLK_m].downThisFrame)
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

                if (playerInput.keyStates[SDLK_z].downThisFrame)
                {
                    showIMGUI = !showIMGUI;
                }
            }

            // change this value to your liking
            float sensitivity = 0.3f;
            playerInput.mouse.pDelta *= sensitivity;

            if (playerInput.keyStates[SDLK_e].downThisFrame)
            {
                g_cursorEngaged = !g_cursorEngaged;
                if (g_cursorEngaged)
                    SDL_ShowCursor(SDL_DISABLE);
                else
                    SDL_ShowCursor(SDL_ENABLE);
            }


            if (showIMGUI)
            {
                ZoneScopedN("ImGui Update");
                {
                    // Start the Dear ImGui frame
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplSDL2_NewFrame(g_window.SDL_Context);
                    ImGui::NewFrame();

                    const float PAD = 5.0f;
                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                    const ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
                    ImVec2 window_pos;//, window_pos_pivot;
                    window_pos.x = work_pos.x + PAD;
                    window_pos.y = work_pos.y + PAD;
                    //window_pos_pivot.x = window_pos_pivot.y = 0.0f;
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, {});

                    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
                    if (ImGui::Begin("Transform Information", nullptr, windowFlags))
                    {
                        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
                        if (ImGui::BeginTable("Position Info", 4, flags))
                        {
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("X");
                            ImGui::TableSetupColumn("Y");
                            ImGui::TableSetupColumn("Z");
                            ImGui::TableHeadersRow();

                            GenericImGuiTable("World", "%+08.2f", player->m_transform.m_p.p.e);
                            GenericImGuiTable("Game", "%i", ToGame(player->m_transform.m_p).p.e);
                            GenericImGuiTable("Chunk", "%i", ToChunk(player->m_transform.m_p).p.e);

                            ImGui::EndTable();
                        }
                        if (ImGui::BeginTable("Movement", 5, flags))
                        {
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("X");
                            ImGui::TableSetupColumn("Y");
                            ImGui::TableSetupColumn("Z");
                            ImGui::TableSetupColumn("W");
                            ImGui::TableHeadersRow();

                            Vec4 vel = { player->m_rigidBody.m_vel.x, player->m_rigidBody.m_vel.y, player->m_rigidBody.m_vel.z, 0.0f };
                            Vec4 accel = { player->m_rigidBody.m_acceleration.x, player->m_rigidBody.m_acceleration.y, player->m_rigidBody.m_acceleration.z, 0.0f };
                            Vec4 rot = { player->m_transform.m_yaw, player->m_transform.m_pitch, 0.0f, 0.0f };
                            GenericImGuiTable("Vel", "%+08.2f", vel.e, 4);
                            GenericImGuiTable("Accel", "%+08.2f", accel.e, 4);
                            //GenericImGuiTable("Quat",  "%+08.2f", player->m_transform.m_quat.e, 4);
                            GenericImGuiTable("Rot", "%+08.2f", rot.e, 4);
                            //GenericImGuiTable("FV",    "%+08.2f", GetVec4(player->GetForwardVector(), 0.0f).e, 4);
                            ImGui::EndTable();
                        }
                        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    }
                    ImGui::End();

                }
                {
                    const float PAD = 5.0f;
                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                    const ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
                    ImVec2 window_pos;//, window_pos_pivot;
                    window_pos.x = work_pos.x + PAD + 320;
                    window_pos.y = work_pos.y + PAD;
                    //window_pos_pivot.x = window_pos_pivot.y = 0.0f;
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, {});

                    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
                    if (ImGui::Begin("Camera Transform Information", nullptr, windowFlags))
                    {
                        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
                        if (ImGui::BeginTable("Position Info", 4, flags))
                        {
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("X");
                            ImGui::TableSetupColumn("Y");
                            ImGui::TableSetupColumn("Z");
                            ImGui::TableHeadersRow();
                            //
                            WorldPos p = playerCamera->GetWorldPosition();
                            GenericImGuiTable("World", "%+08.2f", p.p.e);
                            GenericImGuiTable("Game", "%i", ToGame(p).p.e);
                            GenericImGuiTable("Chunk", "%i", ToChunk(p).p.e);
                            //
                            ImGui::EndTable();
                        }
                        if (ImGui::BeginTable("Movement", 5, flags))
                        {
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("X");
                            ImGui::TableSetupColumn("Y");
                            ImGui::TableSetupColumn("Z");
                            ImGui::TableSetupColumn("W");
                            ImGui::TableHeadersRow();
                            //
                            Vec4 vel = { playerCamera->m_velocity.x,      playerCamera->m_velocity.y,        playerCamera->m_velocity.z, 0.0f };
                            Vec4 rot = { playerCamera->m_transform.m_yaw, playerCamera->m_transform.m_pitch, 0.0f,                       0.0f };
                            GenericImGuiTable("Vel", "%+08.2f", vel.e, 4);
                            //GenericImGuiTable("Quat",  "%+08.2f", playerCamera->m_transform.m_quat.e, 4);
                            GenericImGuiTable("Rot", "%+08.2f", rot.e, 4);
                            ImGui::EndTable();
                        }
                        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    }
                    ImGui::End();
                }

                {
                    ImGui::Begin("Debug");

                    if (ImGui::TreeNode("Drawing"))
                    {
                        ImGui::CheckboxFlags("Enable Debug Drawing", &s_debugFlags, +DebugOptions::Enabled);
                        ImGui::SameLine(); HelpMarker("Key: '`'");
                        //
                        ImGui::Indent();
                        ImGui::CheckboxFlags("All", &s_debugFlags, +DebugOptions::All);
                        //
                        ImGui::Indent();
                        ImGui::CheckboxFlags("Chunk Status", &s_debugFlags, +DebugOptions::ChunkStatus);
                        ImGui::SameLine(); HelpMarker("\
Red:    Unloaded,\n\
Green:  BlocksLoading,\n\
Blue:   BlocksLoaded,\n\
Yellow: VertexLoading,\n\
Purple: VertexLoaded,\n\
White:  Uploaded,");
                        ImGui::CheckboxFlags("Collision Triangles", &s_debugFlags, +DebugOptions::CollisionTriangles);
                        ImGui::CheckboxFlags("Raycast Block", &s_debugFlags, +DebugOptions::LookatBlock);
                        ImGui::CheckboxFlags("Old Raycast Logic", &s_debugFlags, +DebugOptions::OldRaycast);
                        ImGui::Unindent();
                        ImGui::Unindent();
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Game Time"))
                    {
                        if (ImGui::Button("Reset Values"))
                        {
                            g_gameData.m_timeOfDay = TimeOfDay::Afternoon;
                            g_gameData.m_currentTime = s_timesOfDay[+g_gameData.m_timeOfDay];
                            g_gameData.m_timeScale = 1.0f;
                            g_gameData.m_gameTimePlaying = false;
                        }
                        //
                        ImGui::PushItemWidth(100);
                        ImGui::SliderFloat("Current Time", &g_gameData.m_currentTime, 0.0f, 24.0f, "%.2f");
                        ImGui::PopItemWidth();
                        //
                        if (ImGui::Checkbox("Use Time Scale", &g_gameData.m_gameTimePlaying) && !g_gameData.m_gameTimePlaying)
                        {
                            g_gameData.m_currentTime = s_timesOfDay[+g_gameData.m_timeOfDay];
                        }
                        //
                        ImGui::PushItemWidth(100);
                        if (ImGui::SliderInt("Time Of Day", reinterpret_cast<int32*>(&g_gameData.m_timeOfDay), 0, +TimeOfDay::Count - 1, s_timesOfDayNames[+g_gameData.m_timeOfDay]) &&
                            !g_gameData.m_gameTimePlaying)
                        {
                            g_gameData.m_currentTime = s_timesOfDay[+g_gameData.m_timeOfDay];
                        }
                        ImGui::PopItemWidth();
                        //
                        ImGui::PushItemWidth(100);
                        ImGui::SliderFloat("Time Scale", &g_gameData.m_timeScale, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
                        ImGui::PopItemWidth();
                        //
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Misc"))
                    {
                        ImGui::Text("Update the chunks based on:");
                        ImGui::RadioButton("Player", (int32*)&chunkUpdateOrigin, +ChunkUpdateOrigin::Player); ImGui::SameLine();
                        ImGui::RadioButton("Camera", (int32*)&chunkUpdateOrigin, +ChunkUpdateOrigin::Camera); //ImGui::SameLine();
                        ImGui::Spacing();
                        ImGui::Text("Core Count:");
                        ImGui::RadioButton("Multi", (int32*)&multiThreading.threads, +MultiThreading::Threads::multi_thread); ImGui::SameLine();
                        ImGui::RadioButton("Single", (int32*)&multiThreading.threads, +MultiThreading::Threads::single_thread); //ImGui::SameLine();
                        ImGui::Spacing();
                        {
                            //#define RADIO_BUTTON_MACRO(type) 
                            if (ImGui::Button("Add Blocks To Inventory"))
                            {
                                player->m_inventory.Add(ItemToInventoryFromImGUIType, ItemToInventoryFromImGUICount);
                            }
                            ImGui::InputInt("Amount", (int32*)&ItemToInventoryFromImGUICount);
                            ImGui::SameLine(); HelpMarker(
                                "You can apply arithmetic operators +,*,/ on numerical values.\n"
                                "  e.g. [ 100 ], input \'*2\', result becomes [ 200 ]\n"
                                "Use +- to subtract.");

#define RADIO_BUTTON_MACRO(type) ImGui::RadioButton(#type, ((int32*)&ItemToInventoryFromImGUIType), +BlockType::type)
                            RADIO_BUTTON_MACRO(Grass);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Dirt);
                            RADIO_BUTTON_MACRO(Stone);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Planks);
                            RADIO_BUTTON_MACRO(StoneSlab);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Brick);
                            RADIO_BUTTON_MACRO(TNT);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Cobblestone);
                            RADIO_BUTTON_MACRO(Bedrock);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Sand);
                            RADIO_BUTTON_MACRO(Gravel);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Wood);
                            RADIO_BUTTON_MACRO(Snow);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Ice);
                            RADIO_BUTTON_MACRO(Obsidian);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Leaves);
                            RADIO_BUTTON_MACRO(MossyCobblestone);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Water);
                            RADIO_BUTTON_MACRO(HalfSlab);
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Slab);
                            RADIO_BUTTON_MACRO(Glass);
                            //Stone,
                            //Planks,
                            //StoneSlab,
                            //Brick,
//TNT,
//Cobblestone,
//Bedrock,
//Sand,
//Gravel,
//Wood,
//Snow,
//Ice,
//Obsidian,
//Leaves,
//MossyCobblestone,
//Water,
//HalfSlab,
//Slab,
                        }
                        //
                        {
                            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                            if (show_demo_window)
                                ImGui::ShowDemoWindow(&show_demo_window);
                        }
                        //
                        ImGui::TreePop();
                    }
                    ImGui::End();
                }

                {
                    const ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
                    ImVec2 window_pos;//, window_pos_pivot;
                    window_pos.x = (viewport->WorkSize.x / 2) - 175;
                    window_pos.y = viewport->WorkSize.y - 75;
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, {});

                    ImGui::SetNextWindowBgAlpha(0.75f); // Transparent background
                    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
                    ImGui::Begin("Block Hotbar", nullptr, windowFlags);

                    ImGuiTableFlags flags = ImGuiTableFlags_Borders;
                    if (ImGui::BeginTable("Position Info", MAX_SLOTS, flags))
                    {
                        ImU32 hotCellColor = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 1.0f, 0.65f));

                        for (int32 i = 0; i < MAX_SLOTS; i++)
                        {
                            ImGui::TableSetupColumn(ToString("%i", i).c_str());
                            if (i == player->m_inventory.m_slotSelected)
                            {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, hotCellColor);
                            }
                        }

                        ImGui::TableNextRow();
                        for (int32 i = 0; i < MAX_SLOTS; i++)
                        {
                            ImGui::TableSetColumnIndex(i);
                            ImGui::TextUnformatted(ToString("%03i", player->m_inventory.m_slots[i].m_count).c_str());
                            if (i == player->m_inventory.m_slotSelected)
                            {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, hotCellColor);
                            }
                        }

                        //(ImTextureID)(intptr_t)g_FontTexture
                        ImTextureID imMinecraftTextureID = (ImTextureID)(intptr_t)g_renderer.textures[Texture::MinecraftRGB]->m_handle;//ImGui::Image();
                        ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 50% opaque white
                        float sizeOnScreen = 32;

                        ImGui::TableNextRow();
                        for (int32 i = 0; i < MAX_SLOTS; i++)
                        {
                            ImGui::TableSetColumnIndex(i);
                            //ImGui::TextUnformatted(ToString("%2i", playerInventory.m_slots[i].m_block).c_str());
                            auto spriteIndex = 31;
                            if (player->m_inventory.m_slots[i].m_block != BlockType::Empty)
                                spriteIndex = g_blocks[+player->m_inventory.m_slots[i].m_block].m_spriteIndices[+Face::Right];

                            Rect uvResult = GetUVsFromIndex(spriteIndex);
                            ImGui::Image(imMinecraftTextureID, ImVec2(sizeOnScreen, sizeOnScreen),
                                ImVec2(uvResult.botLeft.x, uvResult.botLeft.y),
                                ImVec2(uvResult.topRight.x, uvResult.topRight.y),
                                tint_col, border_col);

                            if (i == player->m_inventory.m_slotSelected)
                            {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, hotCellColor);
                            }
                        }


                        ImGui::EndTable();
                    }

                    ImGui::End();
                }
            }


            if (g_cursorEngaged)
            {
                ZoneScopedN("Entity Input Update");
                g_entityList.InputUpdate(deltaTime, playerInput);
            }


#if 1
            {
                ZoneScopedN("Chunk Items Update");
                g_chunks->Update(deltaTime);
            }
#else
            {
                ZoneScopedN("Items Update");
                g_items.Update(deltaTime);
            }
#endif

            {
                ZoneScopedN("Entity Update");
                g_entityList.Update(deltaTime);
            }


            Vec3 lookTarget = {};
            {
                ZoneScopedN("Sun/Moon Update");
                //Vec3 front = playerCamera->GetTrueRotation() * faceNormals[+Face::Front];
                Vec3 front = playerCamera->GetForwardVector();// (playerCamera->GetWorldMatrix()* g_forwardVectorRotation).xyz;
                //front.x = cos(DegToRad(playerCamera->m_yaw)) * cos(DegToRad(playerCamera->m_pitch));
                //front.y = sin(DegToRad(playerCamera->m_pitch));
                //front.z = sin(DegToRad(playerCamera->m_yaw)) * cos(DegToRad(playerCamera->m_pitch));
                //playerCamera->m_front = Normalize(front);

                //WorldPos cameraRealWorldPosition = playerCamera->GetWorldMatrix() * playerCamera->m_transform.m_p.p;
                WorldPos cameraRealWorldPosition = playerCamera->GetWorldPosition();
                lookTarget = cameraRealWorldPosition.p + front;//playerCamera->m_front;
                gb_mat4_look_at(&playerCamera->m_view, cameraRealWorldPosition.p, lookTarget, playerCamera->m_up);

                float SunRotationRadians = (((g_gameData.m_currentTime - 6.0f) / 24) * tau);
                float sunRotationCos = cosf(SunRotationRadians);
                g_renderer.sunLight.d = Normalize(Vec3({ -sunRotationCos, -sinf(SunRotationRadians),  0.0f }));
                g_renderer.moonLight.d = -g_renderer.sunLight.d;
                const Color sunTransitionColor = { 220 / 255.0f,  90 / 255.0f,  40 / 255.0f, 1.0f };
                const Color moonTransitionColor = { 80 / 255.0f,  80 / 255.0f,  90 / 255.0f, 1.0f };

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
            }
            //END OF GARBAGE?



            GamePos hitBlock;
            bool validHit = false;
            Vec3 hitNormal;

            {
                ZoneScopedN("Raycast");
                //WorldPos cameraRealWorldPosition = playerCamera->RealWorldPos();
                WorldPos cameraRealWorldPosition = playerCamera->GetWorldPosition();//RealWorldPos();
                Ray ray = {
                    .origin = cameraRealWorldPosition.p,
                    .direction = lookTarget - cameraRealWorldPosition.p,
                };
                {
                    ZoneScopedN("RayVsChunk Local");
                    RaycastResult raycast = RayVsChunk(ray, 5.0f);
                    if (raycast.success)
                    {
                        assert(raycast.distance != inf);
                        hitBlock = raycast.p;
                        validHit = (raycast.distance < 5.0f);
                        hitNormal = raycast.normal;
                    }
                }
            }

            if (g_cursorEngaged)
            {
                ZoneScopedN("Player Input Update");
                //Inventory Slot Selection:
                if (playerInput.mouse.wheelInstant.y != 0)
                    player->m_inventory.m_slotSelected = Clamp<int32>(player->m_inventory.m_slotSelected - playerInput.mouse.wheelInstant.y, 0, MAX_SLOTS - 1);

                for (int32 c = SDLK_1; c <= SDLK_8; c++)
                {
                    if (playerInput.keyStates[c].downThisFrame)
                    {
                        assert((c - SDLK_1) < MAX_SLOTS);
                        assert((c - SDLK_1) >= 0);
                        player->m_inventory.m_slotSelected = c - SDLK_1;
                    }
                }

                //Block Deletion and Placement
                //TODO: Optimize to update corners (max 4 chunks)
                if (playerInput.keyStates[SDL_BUTTON_RIGHT].downThisFrame)
                {
                    if (validHit)
                    {
                        BlockType collectedBlockType = BlockType::Empty;
                        if (g_chunks->GetBlock(collectedBlockType, hitBlock))
                        {
                            assert(collectedBlockType != BlockType::Empty);
                            SetBlock(hitBlock, BlockType::Empty);
                            WorldPos itemOrigin = ToWorld(hitBlock).p + 0.5f;


                            ChunkIndex chunkIndex;
                            if (g_chunks->GetChunkFromPosition(chunkIndex, ToChunk(itemOrigin)))
                            {
                                g_items.Add(g_chunks->itemIDs[chunkIndex], collectedBlockType, itemOrigin, playerCamera->GetWorldPosition());
                            }
                            //TODO: add feature to drop block if block could not be picked up
                            //if (auto overflow = player->m_inventory.Add(collectedBlockType, 1))
                            //{
                            //    //do something with the excess
                            //}
                        }
                    }
                }
                else if (playerInput.keyStates[SDL_BUTTON_LEFT].downThisFrame)
                {
                    if (validHit)
                    {
                        GamePos addedBlockPosition;
                        addedBlockPosition.p = { hitBlock.p.x + int32(hitNormal.x), hitBlock.p.y + int32(hitNormal.y), hitBlock.p.z + int32(hitNormal.z) };
                        BlockType addedBlockType;
                        if (g_chunks->GetBlock(addedBlockType, addedBlockPosition))
                        {
                            InventorySlot& slot = player->m_inventory.HotSlot();

                            if (slot.m_count)
                            {
                                assert(slot.m_block != BlockType::Empty);
                                SetBlock(addedBlockPosition, slot.m_block);
                                //Why must i put this stupid uint8 in here for auto to determine the type -_-
                                player->m_inventory.Remove(uint8(1));
                            }
                        }
                    }
                }

                if (playerInput.keyStates[SDLK_x].downThisFrame)
                {
                    InventorySlot& is = player->m_inventory.HotSlot();
                    if (is.m_block != BlockType::Empty)
                    {
                        WorldPos playerPosition = player->GetWorldPosition();
                        Vec3 playerForwardVector = player->GetForwardVector() * 10;
                        WorldPos finalPosition;
                        finalPosition.p = playerPosition.p + playerForwardVector;

                        ChunkIndex chunkIndex;
                        if (g_chunks->GetChunkFromPosition(chunkIndex, ToChunk(player->m_transform.m_p)))
                        {
#if 1
                            WorldPos origin;
                            origin.p = player->m_transform.m_p.p + Vec3{ 0, player->m_collider.m_height / 2, 0 };
                            g_items.Add(g_chunks->itemIDs[chunkIndex], is.m_block, origin, finalPosition);
#else
                            g_items.Add(g_chunks->itemIDs[chunkIndex], is.m_block, player->m_transform.m_p.p, finalPosition);
#endif
                        }
                        player->m_inventory.Remove(uint8(1));
                    }
                }
            }

            //testCamera.p = { 0, 100, 0 };
            //gb_mat4_look_at(&g_camera.view, g_camera.p.p, testCamera.p.p, {0, 1, 0});

            //Vec3 lookatPosition = { (float)sin(totalTime / 10) * 100, 100, (float)cos(totalTime / 10) * 100};
            //gb_mat4_look_at(&testCamera.view, testCamera.p.p, lookatPosition, { 0, 1, 0 });

            gb_mat4_perspective(&playerCamera->m_perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.1f, 2000.0f);
            playerCamera->m_viewProj = playerCamera->m_perspective * playerCamera->m_view;



            {
                ZoneScopedN("Camera Position Chunk Update");

#ifdef _DEBUG
                playerCamera->m_drawDistance = 10;
#elif NDEBUG
                playerCamera->m_drawDistance = 20;//60;
#endif

                playerCamera->m_fogDistance = playerCamera->m_drawDistance + 10;

                ChunkPos chunkPos;
                switch (chunkUpdateOrigin)
                {
                case ChunkUpdateOrigin::Player:
                    chunkPos = ToChunk(player->GetWorldPosition());
                    break;
                case ChunkUpdateOrigin::Camera:
                    chunkPos = ToChunk(playerCamera->GetWorldPosition());
                    break;
                }

                //ZoneScopedN("Distance Check For Chunk");
                for (int32 _drawDistance = 0; _drawDistance < playerCamera->m_drawDistance; _drawDistance++)
                {
                    for (int32 z = -_drawDistance; z <= _drawDistance; z++)
                    {
                        for (int32 x = -_drawDistance; x <= _drawDistance; x++)
                        {
                            if (z == _drawDistance || x == _drawDistance ||
                                z == -_drawDistance || x == -_drawDistance)
                            {
                                ChunkPos newBlockP = { chunkPos.p.x + x, 0, chunkPos.p.z + z };
                                ChunkIndex funcResult;
                                if (TEST_CREATE_AND_UPLOAD_CHUNKS)
                                    if (!g_chunks->GetChunkFromPosition(funcResult, newBlockP))
                                    {
                                        g_chunks->AddChunk(newBlockP);
                                    }
                            }
                        }
                    }
                }

                {
                    ZoneScopedN("Chunk Delete Check");
                    if (playerCamera->m_fogDistance)
                    {
                        for (ChunkIndex i = 0; i < g_chunks->highestActiveChunk; i++)
                        {
                            if (g_chunks->flags[i] & CHUNK_FLAG_ACTIVE)
                            {
                                if (((g_chunks->p[i].p.x > chunkPos.p.x + playerCamera->m_fogDistance || g_chunks->p[i].p.z > chunkPos.p.z + playerCamera->m_fogDistance) ||
                                    (g_chunks->p[i].p.x < chunkPos.p.x - playerCamera->m_fogDistance || g_chunks->p[i].p.z < chunkPos.p.z - playerCamera->m_fogDistance)) ||
                                    !TEST_CREATE_AND_UPLOAD_CHUNKS)
                                {
                                    g_chunks->flags[i] |= CHUNK_FLAG_TODELETE;
                                }
                            }
                        }
                    }
                }
            }

            RenderUpdate(g_window.size, deltaTime);

            //SKYBOX
            {
                ZoneScopedN("Draw Skybox");

                glDepthMask(GL_FALSE);
                ShaderProgram* sp = g_renderer.programs[+Shader::Sun];
                sp->UseShader();
                sp->UpdateUniformVec3("u_directionalLight_d", 1, g_renderer.sunLight.d.e);
                sp->UpdateUniformVec3("u_sunColor", 1, g_renderer.sunLight.c.e);
                sp->UpdateUniformVec3("u_directionalLightMoon_d", 1, g_renderer.moonLight.d.e);
                sp->UpdateUniformVec3("u_moonColor", 1, g_renderer.moonLight.c.e);
                Mat4 iViewProj;
                gb_mat4_inverse(&iViewProj, &playerCamera->m_viewProj);
                sp->UpdateUniformMat4("u_inverseViewProjection", 1, false, iViewProj.e);
                sp->UpdateUniformVec3("u_cameraPosition", 1, playerCamera->GetWorldPosition().p.e);
                sp->UpdateUniformFloat("u_gameTime", g_gameData.m_currentTime);
                glActiveTexture(GL_TEXTURE1);
                g_renderer.skyBoxNight->Bind();
                glActiveTexture(GL_TEXTURE0);
                g_renderer.skyBoxDay->Bind();

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
                glEnableVertexArrayAttrib(g_renderer.vao, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
                glEnableVertexArrayAttrib(g_renderer.vao, 1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
                glEnableVertexArrayAttrib(g_renderer.vao, 2);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glDepthMask(GL_TRUE);
            }

            {
                ZoneScopedN("Semaphore Update");

                for (ChunkIndex i = 0; i < g_chunks->highestActiveChunk; i++)
                {
                    if (!(g_chunks->flags[i] & CHUNK_FLAG_ACTIVE))
                        continue;

                    if (g_chunks->state[i] == ChunkArray::Unloaded)
                    {
                        SetBlocks* job = new SetBlocks();
                        job->chunk = i;
                        g_chunks->state[i] = ChunkArray::BlocksLoading;
                        g_chunks->chunksLoadingBlocks.push_back(i);
                        multiThreading.SubmitJob(job);
                    }
                }
            }

            {
                ZoneScopedN("Chunk Loading Vertex Loop");
                RegionSampler regionSampler = {};
                bool loopSucceeded = false;
                for (auto& i : g_chunks->chunksLoadingBlocks)
                {
                    if (g_chunks->state[i] == ChunkArray::BlocksLoaded)
                    {
                        regionSampler = {};
                        if (regionSampler.RegionGather(i))
                        {
                            CreateVertices* job = new CreateVertices();
                            job->region = regionSampler;

                            g_chunks->state[i] = ChunkArray::VertexLoading;
                            multiThreading.SubmitJob(job);
                            loopSucceeded = true;
                        }
                    }
                }

                if (loopSucceeded)
                {
                    std::erase_if(g_chunks->chunksLoadingBlocks,
                        [](ChunkIndex i)
                        {
                            return g_chunks->state[i] == ChunkArray::VertexLoading;
                        });
                }
            }

            {
                ZoneScopedN("Chunk Deletion");
                for (ChunkIndex i = 0; i < g_chunks->highestActiveChunk; i++)
                {
                    assert(g_chunks->refs[i] >= 0);
                    if (!(g_chunks->flags[i] & CHUNK_FLAG_ACTIVE))
                        continue;
                    if (g_chunks->state[i] == ChunkArray::VertexLoading)
                        continue;
                    if (g_chunks->state[i] == ChunkArray::BlocksLoading)
                        continue;

                    if ((g_chunks->flags[i] & CHUNK_FLAG_TODELETE) && (g_chunks->refs[i] == 0))
                    {
                        g_chunks->ClearChunk(i);
                    }
                }
                while (g_running == false && multiThreading.GetJobsInFlight() > 0)
                    multiThreading.SleepThread(250);  //do nothing;
            }

            {
                ZoneScopedN("Entity Deletion");
                g_entityList.CleanUp();
            }
            {
                ZoneScopedN("Item Deletion");
                g_items.CleanUp();
            }

            struct Renderable {
                ChunkIndex index;
                int32 distance;
            };
            Renderable renderables[MAX_CHUNKS];
            int32 numRenderables = 0;
            {
                ZoneScopedN("Chunk Opaque Render Pass");

#ifdef _DEBUG
                const int32 uploadMax = 10;
#elif NDEBUG
                const int32 uploadMax = 300;
#endif
                Frustum frustum = ComputeFrustum(playerCamera->m_viewProj);
                int32 uploadCount = 0;
                {
                    ZoneScopedN("Pre Opaque Chunk Render");
                    PreOpaqueChunkRender(playerCamera->m_perspective, playerCamera);
                }
                {
                    ZoneScopedN("Adding Renderables");
                    for (ChunkIndex i = 0; i < g_chunks->highestActiveChunk; i++)
                    {
                        if (!(g_chunks->flags[i] & CHUNK_FLAG_ACTIVE))
                            continue;

                        ChunkPos chunkP = g_chunks->p[i];
                        GamePos min = ToGame(chunkP);
                        GamePos max = { min.p.x + (int32)CHUNK_X, min.p.y + (int32)CHUNK_Y, min.p.z + (int32)CHUNK_Z };
                        if (IsBoxInFrustum(frustum, ToWorld(min).p.e, ToWorld(max).p.e))
                        {
                            //renderables[numRenderables].d = ManhattanDistance(ToChunk(playerCamera->RealWorldPos().p).p, chunkP.p);
                            renderables[numRenderables].distance = ManhattanDistance(ToChunk(WorldPos(playerCamera->GetWorldPosition())).p, chunkP.p);
                            renderables[numRenderables++].index = i;
                        }
                    }
                }

                {
                    ZoneScopedN("Distance Sorting Renderables");
                    std::sort(std::begin(renderables), std::begin(renderables) + numRenderables, [](Renderable a, Renderable b) {
                        return a.distance < b.distance;
                        });
                }
                {
                    ZoneScopedN("Upload and Render");
                    for (int32 i = 0; i < numRenderables; i++)
                    {
                        ChunkIndex renderChunk = renderables[i].index;
                        if (g_chunks->state[renderChunk] == ChunkArray::VertexLoaded)
                        {
                            if (uploadCount > uploadMax || (g_chunks->flags[renderChunk] & CHUNK_FLAG_TODELETE))
                                continue;
                            g_chunks->UploadChunk(renderChunk);
                            uploadCount++;
                            uploadedLastFrame = true;
                        }
                        if (g_chunks->state[renderChunk] == ChunkArray::Uploaded)
                        {
                            g_chunks->RenderOpaqueChunk(renderChunk);
                        }
                    }
                }
            }

            {
                ZoneScopedN("Items Opaque Render");
                g_items.RenderOpaque(deltaTime, playerCamera);
            }

            {
                ZoneScopedN("Opaque Debug Code");
                if (s_debugFlags & +DebugOptions::Enabled)
                {
                    for (WorldPos p : cubesToDraw)
                    {
                        g_renderer.opaqueTarget->Bind();
                        //g_renderer.postTarget->Bind();
                        DrawCube(p, Red, 2.0f, playerCamera);
                    }
                }
            }

            {
                ZoneScopedN("Transparent Chunk Render");
                PreTransparentChunkRender(playerCamera->m_perspective, playerCamera);
                {
                    for (int32 i = numRenderables - 1; i >= 0; --i)
                    {
                        ChunkIndex renderChunk = renderables[i].index;
                        if (g_chunks->state[renderChunk] == ChunkArray::Uploaded && g_chunks->translucentIndexCount[renderChunk] > 0)
                        {
                            ZoneScopedN("Transparent Render Per Chunk");
                            g_chunks->RenderTransparentChunk(renderChunk);
                        }
                    }
                }
            }

            {
                ZoneScopedN("Transparent Debug Code");
                //DrawCube(testCamera.p.p, { 0, 1, 0, 1 }, 5.0f, perspective);
                //DrawCube(lookatPosition, { 1, 0, 0, 1 }, 5.0f, perspective);
                if (s_debugFlags & +DebugOptions::Enabled)
                {
                    if (s_debugFlags & +DebugOptions::ChunkStatus)
                    {
                        g_renderer.transparentTarget->Bind();
                        for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
                        {
                            if (!(g_chunks->flags[i] & CHUNK_FLAG_ACTIVE))
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
                            chunkP.p.y = float(g_chunks->height[i] + 1);
                            //chunkP.p.y = CHUNK_Y;
                            chunkP.p.z += CHUNK_Z / 2.0f;
                            Vec3 size = { CHUNK_X / 4.0f, 1, CHUNK_Z / 4.0f };

                            DrawCube(chunkP, colors[static_cast<int32>(g_chunks->state[i])], size, playerCamera);
                        }
                    }
                    if (s_debugFlags & +DebugOptions::LookatBlock)
                    {
                        if (validHit)
                        {
                            g_renderer.transparentTarget->Bind();
                            WorldPos pos;
                            pos = ToWorld(hitBlock);
                            pos.p = pos.p + 0.5f;
                            Color temp = Mint;
                            temp.a = 0.25f;
                            DrawCube(pos, temp, 1.01f, playerCamera);
                        }
                    }
                    if (s_debugFlags & +DebugOptions::CollisionTriangles)
                    {
                        if (player->m_collider.m_collidedTriangles.size())
                        {
                            g_renderer.opaqueTarget->Bind();
                            DrawTriangles(player->m_collider.m_collidedTriangles, Orange, playerCamera->m_view, playerCamera->m_perspective, false);
                            player->m_collider.m_collidedTriangles.clear();
                        }
                    }
                }
            }

            {
                ZoneScopedN("Entity Render");
                g_entityList.Render(deltaTime, playerCamera);
            }

            {
                ZoneScopedN("Items Transparent Render");
                g_items.RenderTransparent(deltaTime, playerCamera);
            }

            //gb_mat4_look_at(&g_camera.view, g_camera.p + a, g_camera.p, { 0,1,0 });

            //double renderTotalTime = SDL_GetPerformanceCounter() / freq;
            //std::erase_if(frameTimes, [renderTotalTime](const float& a)
            //{
            //    return (static_cast<float>(renderTotalTime) - a> 1.0f);
            //});
            //frameTimes.push_back(static_cast<float>(renderTotalTime));

            {
                ZoneScopedN("Transparent Crosshair");
                Color crosshairColor = { 0.5f, 0.5f, 0.5f, 0.5f };
                int32 lineThickness = 7;
                int32 lineBounds = 30;

                Vec2Int screenSizeHalf = g_window.size / 2;

                float screenSizeScale = {};
                Vec2 screenSizeRatio = { g_window.size.x / 2560.0f , g_window.size.y / 1440.0f };
                screenSizeScale = Min(screenSizeRatio.x, screenSizeRatio.y);

                RectInt center;
                {
                    int32 halfLineThickness = int32((lineThickness / 2.0f) * screenSizeScale);
                    center.botLeft = screenSizeHalf - Vec2Int({ halfLineThickness,  halfLineThickness });
                    center.topRight = screenSizeHalf + Vec2Int({ halfLineThickness,  halfLineThickness });
                }

                int32 halfLineBounds = int32((lineBounds / 2.0f) * screenSizeScale);
                RectInt left;
                left.botLeft = { screenSizeHalf.x - halfLineBounds,  center.botLeft.y };
                left.topRight = { center.botLeft.x, center.topRight.y };

                RectInt top;
                top.botLeft = left.topRight;
                top.topRight = { center.topRight.x, screenSizeHalf.y + halfLineBounds };

                RectInt right;
                right.botLeft = { center.topRight.x, center.botLeft.y };
                right.topRight = { screenSizeHalf.x + halfLineBounds,    center.topRight.y };

                RectInt bot;
                bot.botLeft = { center.botLeft.x,  screenSizeHalf.y - halfLineBounds };
                bot.topRight = right.botLeft;

                UI_AddDrawCall({}, center, crosshairColor, Texture::T::Plain);
                UI_AddDrawCall({}, left, crosshairColor, Texture::T::Plain);
                UI_AddDrawCall({}, top, crosshairColor, Texture::T::Plain);
                UI_AddDrawCall({}, right, crosshairColor, Texture::T::Plain);
                UI_AddDrawCall({}, bot, crosshairColor, Texture::T::Plain);

                UI_Render();
            }

            {
                ZoneScopedN("Resolve Framebuffers");
                ResolveMSAAFramebuffer(g_renderer.opaqueTarget, g_renderer.postTarget, (GL_COLOR_BUFFER_BIT));
                ResolveTransparentChunkFrameBuffer();

                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                g_renderer.postTarget->Bind();
                glActiveTexture(GL_TEXTURE0);
                g_renderer.transparentPostTarget->m_color->Bind();
                g_renderer.programs[+Shader::BufferCopy]->UseShader();
                g_renderer.postVertexBuffer->Bind();
            }

            {
                ZoneScopedN("Buffer Copy To Backbuffer");
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                glViewport(0, 0, g_window.size.x, g_window.size.y);
                glActiveTexture(GL_TEXTURE0);
                g_renderer.postTarget->m_color->Bind();
                g_renderer.programs[+Shader::BufferCopy]->UseShader();
                g_renderer.postVertexBuffer->Bind();

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
                glEnableVertexArrayAttrib(g_renderer.vao, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
                glEnableVertexArrayAttrib(g_renderer.vao, 1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
                glEnableVertexArrayAttrib(g_renderer.vao, 2);
                glEnable(GL_FRAMEBUFFER_SRGB);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glDisable(GL_FRAMEBUFFER_SRGB);
            }

            {
                ZoneScopedN("ImGui Render");
                if (showIMGUI)
                {
                    ImGui::Render();
                    //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
                    //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
                    //glClear(GL_COLOR_BUFFER_BIT);
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                }
            }
        }
        {
            ZoneScopedN("Frame End");
            if (g_cursorEngaged)
                SDL_ShowCursor(SDL_DISABLE);

            SDL_GL_SwapWindow(g_window.SDL_Context);
        }
        FrameMark;
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(g_renderer.GL_Context);
    SDL_DestroyWindow(g_window.SDL_Context);
    SDL_Quit();
    return 0;
}
