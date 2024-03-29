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
#include "Vox.h"

#include "imgui.h"
#include "Imgui/imgui_impl_sdl.h"
#include "Imgui/imgui_impl_opengl3.h"
#include "Tracy.hpp"

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
enum class ChunkUpdateOrigin : int32 {
    Player,
    Camera,
};
ENUMOPS(ChunkUpdateOrigin);

enum class DebugOptions : uint32 {
    None = 0,
    ChunkStatus         = BIT(0),
    CollisionTriangles  = BIT(1),
    LookatBlock         = BIT(2),
    Reticle             = BIT(3),
    OldRaycast          = BIT(4),
    Enabled             = BIT(14),
    All = ChunkStatus | CollisionTriangles | LookatBlock | Reticle,
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
    ComplexBlocksInit();
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
    uint32 s_debugFlags = +DebugOptions::LookatBlock | +DebugOptions::Enabled | +DebugOptions::Reticle;
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
    BlockType startHitAddBlockType = BlockType::Empty;
    GamePos   startHitAddBlockP = {};
    float     startHitAddRotation = 0;
    bool      placementCancelled = false;

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
                playerInput.InputUpdate();

                if (playerInput.keyStates[SDLK_0].downThisFrame)
                {
                    CameraReleaseAndCouple(player, playerCamera);
                }

                if (playerInput.keyStates[SDLK_ESCAPE].down)
                    ExitApplication(player, playerCamera);
                if (playerInput.keyStates[SDLK_BACKQUOTE].downThisFrame)
                    s_debugFlags ^= +DebugOptions::Enabled;
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
                float transformInformationWidth = 0.0f;
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
                            Vec3 forward3 = player->GetForwardVector();
                            Vec4 forward = { forward3.x, forward3.y, forward3.z, 0 };
                            GenericImGuiTable("Vel", "%+08.2f", vel.e, 4);
                            GenericImGuiTable("Accel", "%+08.2f", accel.e, 4);
                            //GenericImGuiTable("Quat",  "%+08.2f", player->m_transform.m_quat.e, 4);
                            GenericImGuiTable("Rot", "%+08.2f", rot.e, 4);
                            //GenericImGuiTable("FV",    "%+08.2f", GetVec4(player->GetForwardVector(), 0.0f).e, 4);
                            GenericImGuiTable("for", "%+08.2f", forward.e, 4);
                            ImGui::EndTable();
                        }
                        transformInformationWidth = ImGui::GetWindowSize().x;
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
                    ImGui::SetNextWindowPos(ImVec2(transformInformationWidth + PAD*2, PAD), ImGuiCond_Always);
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
                            Vec3 forward3 = playerCamera->GetForwardVector();
                            Vec4 forward = { forward3.x, forward3.y, forward3.z, 0 };

                            GenericImGuiTable("Vel", "%+08.2f", vel.e, 4);
                            //GenericImGuiTable("Quat",  "%+08.2f", playerCamera->m_transform.m_quat.e, 4);
                            GenericImGuiTable("Rot", "%+08.2f", rot.e, 4);
                            GenericImGuiTable("For", "%+08.2f", forward.e, 4);
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

                        ImGui::Checkbox("MSAA Enabled", &g_renderer.msaaEnabled);
                        ImGui::SameLine(); HelpMarker("'v' will also toggle this");
                        ImGui::Text("Vertical Sync");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-FLT_MIN);
                        if (ImGui::SliderInt("##Vertical Sync", &g_renderer.swapInterval, -1, 1))
                        {
                            SDL_GL_SetSwapInterval(g_renderer.swapInterval);
                        }
                        ImGui::Text("Anisotropic");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-FLT_MIN);
                        ImGui::SliderFloat("##Anisotropic", &g_renderer.currentAnisotropic, 1.0f, g_renderer.maxAnisotropic);
                        ImGui::Text("Depth Peeling Passes");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-FLT_MIN);
                        ImGui::SliderInt("##DepthPeelingPasses", &g_renderer.depthPeelingPasses, 2, 32);
                        ImGui::Text("Depth Peel To Display");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-FLT_MIN);
                        ImGui::SliderInt("##Depth Peel To Display", &g_renderer.debug_DepthPeelingPassToDisplay, -1, g_renderer.depthPeelingPasses);
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
                        if (ImGui::Button("Toggle Camera Attachment"))
                            CameraReleaseAndCouple(player, playerCamera);

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
                                uint8 blocksLeft = player->m_inventory.Add(ItemToInventoryFromImGUIType, ItemToInventoryFromImGUICount);
                                if (blocksLeft)
                                {
                                    ChunkIndex chunkIndex;
                                    if (g_chunks->GetChunkFromPosition(chunkIndex, ToChunk(player->m_transform.m_p)))
                                    {
                                        WorldPos itemOrigin;
                                        itemOrigin.p = player->m_transform.m_p.p;
                                        itemOrigin.p.y += player->m_collider.m_height / 2;
                                        WorldPos itemDestination = itemOrigin;
                                        itemDestination.p += playerCamera->GetForwardVector();
                                        for (int8 i = 0; i < blocksLeft; i++)
                                        {
                                            g_items.Add(g_chunks->itemIDs[chunkIndex], ItemToInventoryFromImGUIType, itemOrigin, itemDestination);
                                        }
                                    }
                                }
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
                            ImGui::SameLine();
                            RADIO_BUTTON_MACRO(Belt);
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
            {
                ZoneScopedN("Chunk Update");
                g_chunks->ItemUpdate(deltaTime);
            }
            {
                ZoneScopedN("Entity Update");
                g_entityList.Update(deltaTime);
            }

            Vec3 lookTarget = {};
            WorldPos cameraRealWorldPosition = playerCamera->GetWorldPosition();
            lookTarget = cameraRealWorldPosition.p + playerCamera->GetForwardVector();
            gb_mat4_look_at(&playerCamera->m_view, cameraRealWorldPosition.p, lookTarget, playerCamera->m_up);

            UpdateHeavens(g_renderer.sunLight, g_renderer.moonLight, g_gameData.m_currentTime);

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
                BlockType hitBlockType = BlockType::Empty;
                ChunkIndex hitChunkIndex = 0;
                if (validHit)
                {
                    if (g_chunks->GetBlock(hitBlockType, hitBlock, hitChunkIndex))
                    {
                        if (g_blocks[+hitBlockType].m_flags & BLOCK_INTERACT)
                        {
                            ChunkPos chunkP;
                            Vec3Int blockP = Convert_GameToBlock(chunkP, hitBlock);
                            ComplexBlock* cb = g_chunks->complexBlocks[hitChunkIndex].GetBlock(blockP);
                            if (cb)
                                cb->OnHover();
                        }
                    }
                    if (playerInput.keyStates[SDLK_f].downThisFrame)
                    {
                        if (hitBlockType != BlockType::Empty)
                        {
                            ChunkPos chunkP = {};
                            ComplexBlock* CB = g_chunks->complexBlocks[hitChunkIndex].GetBlock(Convert_GameToBlock(chunkP, hitBlock));
                            assert(chunkP.p == g_chunks->p[hitChunkIndex].p);
                            if (CB)
                            {
                                BlockType type = player->m_inventory.HotSlot().m_block;
                                if (CB->AddBlock_Offset(type))
                                {
                                    player->m_inventory.Remove(1);
                                }
                            }
                        }
                    }
                }
                if (playerInput.keyStates[SDL_BUTTON_LEFT].down && playerInput.keyStates[SDL_BUTTON_RIGHT].down)
                    placementCancelled = true;
                else if (playerInput.keyStates[SDL_BUTTON_RIGHT].downThisFrame && !placementCancelled)
                {
                    if (hitBlockType != BlockType::Empty)
                    {
                        g_chunks->RemoveBlock(hitBlock, hitBlockType, hitChunkIndex);
                        WorldPos itemOrigin = ToWorld(hitBlock).p + 0.5f;


                        ChunkIndex chunkIndex;
                        if (g_chunks->GetChunkFromPosition(chunkIndex, ToChunk(itemOrigin)))
                        {
                            g_items.Add(g_chunks->itemIDs[chunkIndex], hitBlockType, itemOrigin, playerCamera->GetWorldPosition());
                        }
                    }
                }
                else if (playerInput.keyStates[SDL_BUTTON_LEFT].downThisFrame && !placementCancelled)
                {
                    if (validHit)
                    {
                        startHitAddBlockType = hitBlockType;
                        startHitAddBlockP.p = hitBlock.p + Vec3ToVec3Int(hitNormal);
                        startHitAddRotation = 0;
                    }
                    else
                        placementCancelled = true;
                }


                if ((playerInput.keyStates[SDL_BUTTON_LEFT].down || playerInput.keyStates[SDL_BUTTON_LEFT].upThisFrame)
                    && !placementCancelled && validHit)
                {
                    if (playerInput.keyStates[SDLK_r].downThisFrame && playerInput.keyStates[SDL_BUTTON_LEFT].down)
                    {
                        if (playerInput.keyStates[SDLK_LSHIFT].down || playerInput.keyStates[SDLK_RSHIFT].down)
                            startHitAddRotation -= tau / 4;
                        else
                            startHitAddRotation += tau / 4;
                    }
                    std::vector<GamePos> positions;
                    std::vector<ChunkIndex> chunkIndices;
                    std::vector<BlockType> types;
                    GamePos hitBlockPlusNormal;
                    hitBlockPlusNormal.p = hitBlock.p + Vec3ToVec3Int(hitNormal);
                    Vec3 forwardVector = Vec3IntToVec3(hitBlockPlusNormal.p - startHitAddBlockP.p);
                    if (forwardVector == Vec3({}))
                        forwardVector = playerCamera->GetForwardVector();
                    Mat4 rot;
                    gb_mat4_rotate(&rot, { 0, 1, 0 }, startHitAddRotation);
                    forwardVector = (rot * Vec4({ forwardVector.x, forwardVector.y, forwardVector.z, 1.0f })).xyz;

                    InventorySlot& slot = player->m_inventory.HotSlot();
                    BlockType blockType = slot.m_block;
                    int32 totalBlockCount = slot.m_count;
                    const int32 minx = Min(startHitAddBlockP.p.x, hitBlockPlusNormal.p.x);
                    const int32 miny = Min(startHitAddBlockP.p.y, hitBlockPlusNormal.p.y);
                    const int32 minz = Min(startHitAddBlockP.p.z, hitBlockPlusNormal.p.z);
                    const int32 maxx = Max(startHitAddBlockP.p.x, hitBlockPlusNormal.p.x);
                    const int32 maxy = Max(startHitAddBlockP.p.y, hitBlockPlusNormal.p.y);
                    const int32 maxz = Max(startHitAddBlockP.p.z, hitBlockPlusNormal.p.z);
                    types.push_back(slot.m_block);
                    if (blockType != BlockType::Empty)
                    {
                        for (int32 x = minx; x <= maxx; x++)
                        {
                            for (int32 y = miny; y <= maxy; y++)
                            {
                                for (int32 z = minz; z <= maxz; z++)
                                {
                                    if (!(totalBlockCount))
                                        goto loopEnd;
                                    GamePos addedBlockPosition;
                                    addedBlockPosition.p = { x, y, z };
                                    BlockType currentBlockType;
                                    ChunkIndex chunkIndex;
                                    if (g_chunks->GetBlock(currentBlockType, addedBlockPosition, chunkIndex))
                                    {
                                        if (totalBlockCount && (currentBlockType == BlockType::Empty))
                                        {
                                            BlockSampler sampler;
                                            sampler.RegionGather(addedBlockPosition);
                                            Vec3 unused;
                                            std::vector<Triangle> debug_triangles;
                                            if (CapsuleVsBlock(player->m_collider, sampler, unused, debug_triangles))
                                                continue;

                                            assert(slot.m_block != BlockType::Empty);
                                            if (playerInput.keyStates[SDL_BUTTON_LEFT].upThisFrame)
                                            {
                                                player->m_inventory.Remove(1);
                                                positions.push_back(addedBlockPosition);
                                                chunkIndices.push_back(chunkIndex);
                                            }
                                            else
                                            {
                                                WorldPos p = Vec3IntToVec3(Vec3Int({ x, y, z })) + 0.5f;
                                                AddBlockToRender(p, 1.0f, blockType, transCyan, forwardVector);
                                            }
                                            totalBlockCount--;
                                        }
                                    }
                                }
                            }
                        }
                    }
                loopEnd: {}
                    if (playerInput.keyStates[SDL_BUTTON_LEFT].upThisFrame)
                        g_chunks->AddBlockMultiple(positions, chunkIndices, types, forwardVector);
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

                if ((!playerInput.keyStates[SDL_BUTTON_LEFT].down) && (!playerInput.keyStates[SDL_BUTTON_RIGHT].down))
                    placementCancelled = false;
            }

            //Near Clip and Far Clip
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

                g_chunks->Update(chunkPos, playerCamera->m_drawDistance, playerCamera->m_fogDistance, multiThreading);
            }


            {
                ZoneScopedN("Entity Deletion");
                g_entityList.CleanUp();
            }
            {
                ZoneScopedN("Item Deletion");
                g_items.CleanUp();
            }
            {
                ZoneScopedN("Chunk Clean Up");
                g_chunks->CleanUp();
            }



            //Debug Checks
            if (s_debugFlags & +DebugOptions::Enabled)
            {
                if (s_debugFlags & +DebugOptions::ChunkStatus)
                {
                    g_framebuffers->m_transparent.Bind();
                    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
                    {
                        if (!(g_chunks->flags[i] & CHUNK_FLAG_ACTIVE))
                            continue;

                        Color colors[] = {
                            { 1, 0, 0, 0.6f },//Red    //Unloaded,
                            { 0, 1, 0, 0.6f },//Green  //BlocksLoading,
                            { 0, 0, 1, 0.6f },//Blue   //BlocksLoaded,
                            { 1, 1, 0, 0.6f },//Yellow //VertexLoading,
                            { 1, 0, 1, 0.6f },//Purple //VertexLoaded,
                            { 1, 1, 1, 0.6f },//White  //Uploaded,
                        };

                        WorldPos chunkP = ToWorld(Convert_ChunkIndexToGame(i));
                        chunkP.p.x += CHUNK_X / 2.0f;
                        chunkP.p.y = float(g_chunks->height[i] + 1);
                        //chunkP.p.y = CHUNK_Y;
                        chunkP.p.z += CHUNK_Z / 2.0f;
                        Vec3 size = { CHUNK_X / 4.0f, 1, CHUNK_Z / 4.0f };

                        AddCubeToRender(chunkP, colors[static_cast<int32>(g_chunks->state[i])], size);
                    }
                }
                if (s_debugFlags & +DebugOptions::LookatBlock)
                {
                    if (validHit)
                    {
                        g_framebuffers->m_transparent.Bind();
                        WorldPos pos;
                        pos = ToWorld(hitBlock);
                        pos.p = pos.p + (g_blocks[+BlockType::Grass].m_size / 2.0f);
                        Color temp = Mint;
                        temp.a = 0.15f;
                        AddCubeToRender(pos, temp, 1.01f);
                    }
                }
                if (s_debugFlags & +DebugOptions::Reticle)
                {
                    auto draw = ImGui::GetBackgroundDrawList();
                    ImVec2  center  = { g_window.size.x / 2.0f, g_window.size.y / 2.0f };
                    //float   size    = 2.0f;
                    float   size    = Max(g_window.size.x / 640.0f, 2.0f);
                    ImU32   color   = IM_COL32(127, 127, 127, 127);
                    ImU32   white   = IM_COL32(255, 255, 255, 255);
                    draw->AddCircleFilled(center, size, color);
                    draw->AddCircleFilled(center, Max(size / 4, 1.0f), white);
                }
            }
#if 0
            {
                WorldPos cubePosition;
                cubePosition.p = {-125, 200, 0};
                AddCubeToRender(cubePosition, White, 1);
                cubePosition.p.y -= 1;
                AddCubeToRender(cubePosition, Blue, 0.5f);
                cubePosition.p.y -= 1;
                AddCubeToRender(cubePosition, Orange, 0.25f);
            }
#endif




            RenderUpdate(g_window.size, deltaTime);

            //SKYBOX
            RenderSkybox(playerCamera);

            struct Renderable {
                ChunkIndex index;
                int32 distance;
            };
            Renderable renderables[MAX_CHUNKS];
            int32 numRenderables = 0;
            {
                ZoneScopedN("Rendering");
                assert(g_renderer.depthPeelingPasses);

                //Chunk Rendering
#ifdef _DEBUG
                const int32 uploadMax = 10;
#elif NDEBUG
                const int32 uploadMax = 300;
#endif
                Frustum frustum = ComputeFrustum(playerCamera->m_viewProj);
                int32 uploadCount = 0;
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


                //
                // Opaque Pass
                //
                {
                    const auto& renderTarget = g_framebuffers->m_opaque;
                    ZoneScopedN("Opaque Pass");
                    renderTarget.Bind();

                    DepthWrite(true);
                    DepthRead(true);
                    glDepthFunc(GL_LESS);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    PreOpaqueChunkRender(playerCamera->m_perspective, playerCamera, 0);

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
                            g_chunks->RenderChunkOpaquePeel(renderChunk);
                        }
                    }
                    {
                        ZoneScopedN("Chunk Complex Block Render");
                        g_chunks->RenderChunkOpaqueChildren(playerCamera, 0);
                    }

                    {
                        ZoneScopedN("Render Entity");
                        g_entityList.Render(deltaTime, playerCamera);
                    }

                    RenderOpaqueBlocks(playerCamera, 0);
                    RenderOpaqueCubes(playerCamera, 0);


                    g_framebuffers->m_resolveDepthPeeling.Bind();
                    Texture* colorBuffer = g_framebuffers->m_resolveDepthPeeling.m_opaqueColor;
                    Texture* depthBuffer = g_framebuffers->m_resolveDepthPeeling.m_opaqueDepth;
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer->m_target, colorBuffer->m_handle, 0);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthBuffer->m_target, depthBuffer->m_handle, 0);// this doesnt change
                    glClearColor(0, 0, 0, 0);
                    glClearDepth(1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    ResolveMSAAFramebuffer(&renderTarget, &g_framebuffers->m_resolveDepthPeeling, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                }



                //
                // Prepare for depth peeling loop
                //
                {
                    ZoneScopedN("Transparent Setup");
                    const auto& renderTarget = g_framebuffers->m_transparent;
                    //Clear color buffers before first pass
                    {
                        g_framebuffers->m_resolveDepthPeeling.Bind();
                        for (int32 pass = 0; pass < g_renderer.depthPeelingPasses; pass++)
                        {
                            Texture* color = g_framebuffers->m_resolveDepthPeeling.m_peelingColors[pass];
                            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color->m_target, color->m_handle, 0);
                            glClear(GL_COLOR_BUFFER_BIT);
                        }
                    }
                    //Set the second texture in the depth peeling scene to the previous scene's depth
                    //Set the third  texture in the depth peeling scene to the opaque scene's depth
                    {
                        renderTarget.Bind();
                        glActiveTexture(GL_TEXTURE1);
                        g_framebuffers->m_resolveDepthPeeling.m_peelingDepth->Bind();
                        glActiveTexture(GL_TEXTURE2);
                        g_framebuffers->m_resolveDepthPeeling.m_opaqueDepth->Bind();
                    }
                }



                //
                // Depth peeling loop
                //
                {
                    ZoneScopedN("Transparent Pass");
                    glDisable(GL_BLEND);
                    for (int32 pass = 0; pass < g_renderer.depthPeelingPasses; pass++)
                    {
                        const auto& renderTarget = g_framebuffers->m_transparent;
                        ZoneScopedN("Depth Peeling Pass");
                        std::string loopInformation = ToString("DepthPass Number: %i", pass);
                        ZoneText(loopInformation.c_str(), loopInformation.size());

                        renderTarget.Bind();
                        DepthWrite(true);
                        DepthRead(true);
                        glDepthFunc(GL_LESS);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


                        //Render the blocks
                        PreOpaqueChunkRender(playerCamera->m_perspective, playerCamera, pass + 1);
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
                                    g_chunks->RenderChunkTransparentPeel(renderChunk);
                                }
                            }
                        }

                        bool lastPass = (pass == (g_renderer.depthPeelingPasses - 1));
                        RenderTransparentBlocks(playerCamera, pass + 1, lastPass);
                        RenderTransparentCubes(playerCamera, pass + 1, lastPass);

                        {
                            ZoneScopedN("Debug Rendering");
                            //DrawCube(testCamera.p.p, { 0, 1, 0, 1 }, 5.0f, perspective);
                            //DrawCube(lookatPosition, { 1, 0, 0, 1 }, 5.0f, perspective);
                            if (s_debugFlags & +DebugOptions::Enabled)
                            {
                                if (s_debugFlags & +DebugOptions::CollisionTriangles)
                                {
                                    if (player->m_collider.m_collidedTriangles.size())
                                    {
                                        DrawTriangles(player->m_collider.m_collidedTriangles, Orange, playerCamera->m_view, playerCamera->m_perspective, false, pass + 1);
                                    }
                                }
                                player->m_collider.m_collidedTriangles.clear();
                            }
                        }



                        //Resolve MSAA depth buffer into non-MSAA texture buffer
                        {
                            g_framebuffers->m_resolveDepthPeeling.Bind();
                            Texture* colorBuffer = g_framebuffers->m_resolveDepthPeeling.m_peelingColors[pass];
                            Texture* depthBuffer = g_framebuffers->m_resolveDepthPeeling.m_peelingDepth;
                            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer->m_target, colorBuffer->m_handle, 0);
                            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthBuffer->m_target, depthBuffer->m_handle, 0);// this doesnt change

                            ResolveMSAAFramebuffer(&renderTarget, &g_framebuffers->m_resolveDepthPeeling, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                        }
                    }
                    glEnable(GL_BLEND);
                }

                //Composite the ResolveDepthPeelingTarget m_colors into a single color buffer on postTarget
                {
                    auto& depthPeels = g_framebuffers->m_resolveDepthPeeling;

                    DepthRead(false);
                    DepthWrite(false);

                    glEnable(GL_FRAMEBUFFER_SRGB);

                    if (g_renderer.debug_DepthPeelingPassToDisplay == -1)
                    {
                        glActiveTexture(GL_TEXTURE0);
                        depthPeels.m_opaqueColor->Bind();

                        RenderAlphaCopy(depthPeels.m_opaqueColor, g_framebuffers->m_post.m_color);

                        for (int32 pass = g_renderer.depthPeelingPasses; pass; --pass)
                        {
                            RenderAlphaCopy(depthPeels.m_peelingColors[pass - 1], g_framebuffers->m_post.m_color);
                        }
                    }
                    else if (g_renderer.debug_DepthPeelingPassToDisplay == 0)
                    {
                        RenderAlphaCopy(depthPeels.m_opaqueColor, g_framebuffers->m_post.m_color);
                    }
                    else
                    {
                        assert(g_renderer.debug_DepthPeelingPassToDisplay > 0);
                        assert(g_renderer.debug_DepthPeelingPassToDisplay <= g_renderer.depthPeelingPasses);
                        RenderAlphaCopy(depthPeels.m_peelingColors[g_renderer.debug_DepthPeelingPassToDisplay - 1], g_framebuffers->m_post.m_color);
                    }
                    glDisable(GL_FRAMEBUFFER_SRGB);
                }
                {
                    ZoneScopedN("Buffer Copy To Backbuffer");
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_BLEND);
                    glViewport(0, 0, g_window.size.x, g_window.size.y);
                    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glActiveTexture(GL_TEXTURE0);
                    g_framebuffers->m_post.m_color->Bind();
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
            }
            {
                ZoneScopedN("ImGui Render");
                if (showIMGUI)
                {
                    ImGui::Render();
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
