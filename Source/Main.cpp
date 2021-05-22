#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define _DEBUGPRINT
#include "SDL.h"
#include "Math.h"
#include "glew.h"
#include "STB/stb_image.h"
#include "Misc.h"
#include "Rendering.h"
#include "Block.h"
#include "Computer.h"
#include "WinInterop.h"
#include "Noise.h"
#include "Input.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

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

enum class TimeOfDay : int32 {
    Midnight,
    Morning,
    Afternoon,
    Evening,
    Count,
};
ENUMOPS(TimeOfDay);
float s_timesOfDay[+TimeOfDay::Count] = { 0.0f, 7.0f, 10.0f, 17.0f };
const char* s_timesOfDayNames[+TimeOfDay::Count] = { "Midnight", "Morning", "Afternoon", "Evening" };

struct GameData {
    float m_currentTime = 12.0f;
    float m_timeScale = 1.0f;
    TimeOfDay m_timeOfDay = TimeOfDay::Afternoon;
    bool m_gameTimePlaying = false;
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

enum class MovementType {
    Fly,
    Collision,
};

enum class DebugOptions : uint32 {
    None = 0,
    ChunkStatus = BIT(0),
    CollisionTriangles = BIT(1),
    LookatBlock = BIT(2),
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

int main(int argc, char* argv[])
{
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
    gb_mat4_look_at(&g_camera.view, { g_camera.transform.m_p.p.x + cOffset.p.x, g_camera.transform.m_p.p.y + cOffset.p.y,g_camera.transform.m_p.p.z + cOffset.p.z },
                                    { g_camera.transform.m_p.p.x, g_camera.transform.m_p.p.y, g_camera.transform.m_p.p.z }, { 0,1,0 });

    g_chunks = new ChunkArray();
    float loadingTimer = 0.0f;
    bool uploadedLastFrame = false;
    //bool debugDraw = false;
    uint32 s_debugFlags = +DebugOptions::LookatBlock | +DebugOptions::Enabled;
    bool TEST_CREATE_AND_UPLOAD_CHUNKS = true;


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

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    ImGuiIO& imGuiIO = ImGui::GetIO();

    Capsule playerCollider = {
    .m_radius = 0.25f,
    .m_height = 1.8f,
    };
    MovementType playerMovementType = MovementType::Fly;

    bool g_cursorEngaged = true;

    cubesToDraw.reserve(100000);
    while (g_running)
    {
        PROFILE_SCOPE_TAB("FRAMETIME:");
        totalTime = SDL_GetPerformanceCounter() / freq;
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
        Vec2Int originalMouseLocation = {};
        Vec2Int newMouseLocation = {};


        g_mouse.pDelta = { 0, 0 };

        if (multiThreading.GetJobsInFlight() > 0 || uploadedLastFrame)
            loadingTimer += deltaTime;
        uploadedLastFrame = false;
        {
            ChunkPos cameraChunk = ToChunk(g_camera.transform.m_p);
            SDL_SetWindowTitle(g_window.SDL_Context, ToString("TooSquared V: %0.2f, %0.2f, %0.2f; Chunks: %u; Time: %0.2f; Triangles: %u; grounded: %i",
                g_camera.transform.m_vel.x, g_camera.transform.m_vel.y, g_camera.transform.m_vel.z,
                g_chunks->chunkCount,
                loadingTimer, g_renderer.numTrianglesDrawn, g_camera.transform.m_isGrounded).c_str());
        }



        SDL_Event SDLEvent;
        g_mouse.pDelta = {};
        while (SDL_PollEvent(&SDLEvent))
        {
            ImGui_ImplSDL2_ProcessEvent(&SDLEvent);

            switch (SDLEvent.type)
            {
            case SDL_QUIT:
                g_running = false;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (g_window.hasAttention)
                {
                    if (!imGuiIO.WantCaptureKeyboard)
                    {
                        keyStates[SDLEvent.key.keysym.sym].down = (SDLEvent.type == SDL_KEYDOWN);
                    }
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (g_window.hasAttention)
                {
                    if(!imGuiIO.WantCaptureMouse)
                    {
                        keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
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
                            g_mouse.pDelta.x += (static_cast<float>(SDLEvent.motion.x) - g_mouse.pos.x);
                            g_mouse.pDelta.y += (static_cast<float>(SDLEvent.motion.y) - g_mouse.pos.y);// reversed since y-coordinates go from bottom to top

                            SDL_WarpMouseInWindow(g_window.SDL_Context, g_window.size.x / 2, g_window.size.y / 2);
                            //g_mouse.pos.x = SDLEvent.motion.x;
                            //g_mouse.pos.y = SDLEvent.motion.y;
                            g_mouse.pos.x = g_window.size.x / 2;
                            g_mouse.pos.y = g_window.size.y / 2;
                        }

                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                if (g_window.hasAttention)
                {
                    if(!imGuiIO.WantCaptureMouse)
                    {
                        g_mouse.wheel.x = SDLEvent.wheel.x;
                        g_mouse.wheel.y = SDLEvent.wheel.y;
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
                    g_mouse.pDelta = {};
                    SDL_GetMouseState(&g_mouse.pos.x, &g_mouse.pos.y);
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
            s_debugFlags ^= +DebugOptions::Enabled;
        if (keyStates[SDLK_0].downThisFrame)
        {
            switch (playerMovementType)
            {
            case MovementType::Fly:
                playerMovementType = MovementType::Collision;
                break;
            case MovementType::Collision:
                playerMovementType = MovementType::Fly;
                break;
            }
        }

        if (keyStates[SDLK_c].downThisFrame)
            TEST_CREATE_AND_UPLOAD_CHUNKS = !TEST_CREATE_AND_UPLOAD_CHUNKS;
        if (keyStates[SDLK_v].downThisFrame)
            g_renderer.msaaEnabled = !g_renderer.msaaEnabled;
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


        //if (keyStates[SDL_BUTTON_LEFT].down)
        {
            float mouseSensativity = 0.4f;
            g_camera.yaw += g_mouse.pDelta.x * mouseSensativity;
            g_camera.pitch -= g_mouse.pDelta.y * mouseSensativity;
        }

        if (keyStates[SDLK_e].downThisFrame)
        {
            g_cursorEngaged = !g_cursorEngaged;
            if (g_cursorEngaged)
                SDL_ShowCursor(SDL_DISABLE);
            else
                SDL_ShowCursor(SDL_ENABLE);
        }

        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(g_window.SDL_Context);
            ImGui::NewFrame();

#if 1
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

                    GenericImGuiTable("World", "%+08.2f", g_camera.transform.m_p.p.e);
                    GamePos cameraGameP = ToGame(g_camera.transform.m_p);
                    GenericImGuiTable("Game", "%i", cameraGameP.p.e);
                    ChunkPos cameraChunk = ToChunk(g_camera.transform.m_p);
                    GenericImGuiTable("Chunk", "%i", cameraChunk.p.e);

                    ImGui::EndTable();
                }
                if (ImGui::BeginTable("Movement", 4, flags))
                {
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("X");
                    ImGui::TableSetupColumn("Y");
                    ImGui::TableSetupColumn("Z");
                    ImGui::TableHeadersRow();

                    GenericImGuiTable("Vel",   "%+08.2f", g_camera.transform.m_vel.e);
                    GenericImGuiTable("Accel", "%+08.2f", g_camera.transform.m_acceleration.e);
                    ImGui::EndTable();
                }
                ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }
            ImGui::End();

#else
            ImGui::Begin("Transform Information");
            ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
            if (ImGui::BeginTable("Position Info", 4, flags))
            {
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("X");
                ImGui::TableSetupColumn("Y");
                ImGui::TableSetupColumn("Z");
                ImGui::TableHeadersRow();

                GenericImGuiTable("World", "%.2f", g_camera.transform.m_p.p.e);
                GamePos cameraGameP = ToGame(g_camera.transform.m_p);
                GenericImGuiTable("Game", "%i", cameraGameP.p.e);
                ChunkPos cameraChunk = ToChunk(g_camera.transform.m_p);
                GenericImGuiTable("Chunk", "%i", cameraChunk.p.e);

                ImGui::EndTable();
            }
            if (ImGui::BeginTable("Movement", 4, flags))
            {
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("X");
                ImGui::TableSetupColumn("Y");
                ImGui::TableSetupColumn("Z");
                ImGui::TableHeadersRow();

                GenericImGuiTable("Vel", "%.2f", g_camera.transform.m_vel.e);
                GenericImGuiTable("Accel", "%.2f", g_camera.transform.m_acceleration.e);
                ImGui::EndTable();
            }
            ImGui::End();
#endif
        }

        {
            ImGui::Begin("Debug");

            if (ImGui::TreeNode("Drawing"))
            {
                ImGui::CheckboxFlags("Enable Debug Drawing", &s_debugFlags, +DebugOptions::Enabled);
                ImGui::SameLine(); HelpMarker("Key: '`'");

                ImGui::Indent();
                ImGui::CheckboxFlags("All", &s_debugFlags, +DebugOptions::All);

                ImGui::Indent();
                ImGui::CheckboxFlags("Chunk Status", &s_debugFlags, +DebugOptions::ChunkStatus);
                ImGui::CheckboxFlags("Collision Triangles", &s_debugFlags, +DebugOptions::CollisionTriangles);
                ImGui::CheckboxFlags("Raycast Block", &s_debugFlags, +DebugOptions::LookatBlock);
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

                ImGui::PushItemWidth(100);
                ImGui::SliderFloat("Current Time", &g_gameData.m_currentTime, 0.0f, 24.0f, "%.2f");
                ImGui::PopItemWidth();

                if (ImGui::Checkbox("Use Time Scale", &g_gameData.m_gameTimePlaying) && !g_gameData.m_gameTimePlaying)
                {
                    g_gameData.m_currentTime = s_timesOfDay[+g_gameData.m_timeOfDay];
                }

                ImGui::PushItemWidth(100);
                if (ImGui::SliderInt("Time Of Day", reinterpret_cast<int32*>(&g_gameData.m_timeOfDay), 0, +TimeOfDay::Count - 1, s_timesOfDayNames[+g_gameData.m_timeOfDay]) &&
                    !g_gameData.m_gameTimePlaying)
                {
                    g_gameData.m_currentTime = s_timesOfDay[+g_gameData.m_timeOfDay];
                }
                ImGui::PopItemWidth();

                ImGui::PushItemWidth(100);
                ImGui::SliderFloat("Time Scale", &g_gameData.m_timeScale, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
                ImGui::PopItemWidth();

                ImGui::TreePop();
            }
            ImGui::End();
        }

        {
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }

        }


        float cameraAcceleration = 0;
        g_camera.transform.m_acceleration = {};
        std::vector<Triangle> debug_trianglesToDraw;
        {
            switch (playerMovementType)
            {
            case MovementType::Fly:
                cameraAcceleration = 100.0f; // m/s^2
                g_camera.transform.m_terminalVel.x = g_camera.transform.m_terminalVel.z = 20.0f;
                g_camera.transform.m_terminalVel.y = 500.0f;
                if (keyStates[SDLK_LSHIFT].down)
                {
                    cameraAcceleration *= 30;
                    g_camera.transform.m_terminalVel.x = g_camera.transform.m_terminalVel.z = 800.0f;
                }
                {
                    //Forward
                    if (keyStates[SDLK_w].down && keyStates[SDLK_s].down)
                        g_camera.transform.m_acceleration;
                    else if (keyStates[SDLK_w].down)
                        g_camera.transform.m_acceleration += (cameraAcceleration * g_camera.front);
                    else if (keyStates[SDLK_s].down)
                        g_camera.transform.m_acceleration -= (cameraAcceleration * g_camera.front);
                }
                {
                    //Lateral
                    if (keyStates[SDLK_a].down && keyStates[SDLK_d].down)
                        g_camera.transform.m_acceleration;
                    else if (keyStates[SDLK_a].down)
                        g_camera.transform.m_acceleration -= (Normalize(Cross(g_camera.front, g_camera.up)) * cameraAcceleration);
                    else if (keyStates[SDLK_d].down)
                        g_camera.transform.m_acceleration += (Normalize(Cross(g_camera.front, g_camera.up)) * cameraAcceleration);
                }
                if (keyStates[SDLK_LCTRL].down)
                    g_camera.transform.m_terminalVel.x = g_camera.transform.m_terminalVel.z = 10.0f;
                if (keyStates[SDLK_SPACE].down)
                    g_camera.transform.m_acceleration.y += cameraAcceleration;
                if (keyStates[SDLK_z].down)
                    g_camera.transform.m_acceleration.z += cameraAcceleration;
                if (keyStates[SDLK_x].down)
                    g_camera.transform.m_acceleration.x += cameraAcceleration;
                g_camera.transform.UpdatePosition(deltaTime, { 0.9f, 0.9f, 0.9f }, playerCollider.m_radius * 2 * playerCollider.m_height, 0);
                break;
            case MovementType::Collision:
            {
                cameraAcceleration = 15.0f; // m/s^2
                Vec3 forward = Normalize(Vec3({ g_camera.front.x, 0, g_camera.front.z }));
                g_camera.transform.m_terminalVel.x = g_camera.transform.m_terminalVel.z = 3.0f;
                g_camera.transform.m_terminalVel.y = 50.0f;

                if (keyStates[SDLK_LSHIFT].down)
                {
                    cameraAcceleration = 50.0f;
                    g_camera.transform.m_terminalVel.x = g_camera.transform.m_terminalVel.z = 8.0f;
                }
                if (keyStates[SDLK_LCTRL].down)
                    cameraAcceleration /= 3.0f;
                {
                    //Forward
                    if (keyStates[SDLK_w].down && keyStates[SDLK_s].down)
                        g_camera.transform.m_acceleration;
                    else if (keyStates[SDLK_w].down)
                        g_camera.transform.m_acceleration += (cameraAcceleration * forward);
                    else if (keyStates[SDLK_s].down)
                        g_camera.transform.m_acceleration -= (cameraAcceleration * forward);
                }
                {
                    //Lateral
                    if (keyStates[SDLK_a].down && keyStates[SDLK_d].down)
                        g_camera.transform.m_acceleration;
                    else if (keyStates[SDLK_a].down)
                        g_camera.transform.m_acceleration -= (Normalize(Cross(forward, g_camera.up)) * cameraAcceleration);
                    else if (keyStates[SDLK_d].down)
                        g_camera.transform.m_acceleration += (Normalize(Cross(forward, g_camera.up)) * cameraAcceleration);
                }
                if (keyStates[SDLK_SPACE].downThisFrame)
                {
                    g_camera.transform.m_vel.y += 7.0f;
                    g_camera.transform.m_isGrounded = false;
                }
                if (keyStates[SDLK_z].down)
                    g_camera.transform.m_acceleration.z += cameraAcceleration;
                if (keyStates[SDLK_x].down)
                    g_camera.transform.m_acceleration.x += cameraAcceleration;

                g_camera.transform.UpdateDeltaPosition(deltaTime, { 10.0f, 1.0f, 10.0f }, playerCollider.m_radius * 2 * playerCollider.m_height);
                playerCollider.UpdateMidTipLocation(g_camera.transform.m_p);
                //g_camera.transform.UpdatePosition2(deltaTime, { 10.0f, 1.0f, 10.0f }, playerCollider.m_radius * 2 * playerCollider.m_height);

                g_camera.transform.m_p.p += g_camera.transform.m_pDelta.p;
                Vec3 deltaPosition = {};
                g_camera.transform.m_isGrounded = false;
                if (CapsuleVsWorldBlocks(playerCollider, g_camera.transform.m_pDelta.p, deltaPosition, debug_trianglesToDraw))
                {
                    //Update Position
                    g_camera.transform.m_p.p += deltaPosition;
                    g_camera.transform.m_pDelta = {};

                    //Zero velocity going into a collision
                    Vec3 normalForceDirection = Normalize(deltaPosition);
                    Vec3 collisionDirection = normalForceDirection;//-normalForceDirection;
                    Vec3 dotProductResults = { DotProduct(Vec3({ g_camera.transform.m_vel.x, 0.0f, 0.0f }), collisionDirection),
                                               DotProduct(Vec3({ 0.0f, g_camera.transform.m_vel.y, 0.0f }), collisionDirection),
                                               DotProduct(Vec3({ 0.0f, 0.0f, g_camera.transform.m_vel.z }), collisionDirection) };

                    //TODO: improve to include deflection/angle of collision not just collision in that direction
                    if (dotProductResults.x < 0.0f)
                    {
                        g_camera.transform.m_vel.x = 0.0f;
                    }
                    if (dotProductResults.y < 0.0f)
                    {
                        if (g_camera.transform.m_vel.y < 0.0f)
                            g_camera.transform.m_isGrounded = true;
                        g_camera.transform.m_vel.y = 0.0f;
                    }
                    if (dotProductResults.z < 0.0f)
                    {
                        g_camera.transform.m_vel.z = 0.0f;
                    }
                }


                break;
            }
            }
        }


        //make sure that when pitch is out of bounds, screen doesn't get flipped
        g_camera.pitch = Clamp<float>(g_camera.pitch, -89.0f, 89.0f);
        Vec3 lookTarget = {};
        {
            Vec3 front = {};
            front.x = cos(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
            front.y = sin(DegToRad(g_camera.pitch));
            front.z = sin(DegToRad(g_camera.yaw)) * cos(DegToRad(g_camera.pitch));
            g_camera.front = Normalize(front);

            lookTarget = g_camera.transform.m_p.p + g_camera.front;
            gb_mat4_look_at(&g_camera.view, g_camera.transform.m_p.p, lookTarget, g_camera.up);

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
            PROFILE_SCOPE_TAB("Raycast");
            RegionSampler localRegion;
            ChunkIndex centerChunkIndex;
            Ray ray = {
                .origin = g_camera.transform.m_p.p,
                .direction = lookTarget - g_camera.transform.m_p.p,
            };
            if (g_chunks->GetChunkFromPosition(centerChunkIndex, ToChunk(g_camera.transform.m_p)))
            {
                //bool RayVsChunk(const Ray & ray, ChunkIndex chunkIndex, GamePos & block, float& distance);
                GamePos resultPos = {};
                float distance;
                {
                    PROFILE_SCOPE_TAB("RayVsChunk");
                    if (RayVsChunk(ray, centerChunkIndex, resultPos, distance, hitNormal))
                    {
                        hitBlock = resultPos;
                        validHit = (distance < 5.0f);
                    }
                }

                PROFILE_SCOPE_TAB("Ray Neighbor gather and loop");
                if (!validHit && localRegion.RegionGather(centerChunkIndex))
                {
                    PROFILE_SCOPE_TAB("Ray Neighbor loop");
                    for (ChunkIndex neighbor : localRegion.neighbors)
                    {
                        float distanceComparison;
                        Vec3 neighborNormal;
                        PROFILE_SCOPE_TAB("RayVsChunk2");
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
                SetBlock(hitBlock, BlockType::Empty);
            }
        }
        //TODO: Optimize to update corners (max 4 chunks)
        for (int32 c = SDLK_1; c <= SDLK_9; c++)
        {
            //if (keyStates[c].downThisFrame)
            if (keyStates[c].downThisFrame)
            {
                if (validHit)
                {
                    //SetBlock(hitBlock, hitNormal, BlockType(c - SDLK_1 + 1));
                    SetBlock({ hitBlock.p.x + int32(hitNormal.x), hitBlock.p.y + int32(hitNormal.y), hitBlock.p.z + int32(hitNormal.z) }, BlockType(c - SDLK_1 + 1));
                    break;
                }
            }
        }

        //testCamera.p = { 0, 100, 0 };
        //gb_mat4_look_at(&g_camera.view, g_camera.p.p, testCamera.p.p, {0, 1, 0});

        //Vec3 lookatPosition = { (float)sin(totalTime / 10) * 100, 100, (float)cos(totalTime / 10) * 100};
        //gb_mat4_look_at(&testCamera.view, testCamera.p.p, lookatPosition, { 0, 1, 0 });

        Mat4 perspective;
        gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.1f, 2000.0f);
        Mat4 viewProj = perspective * g_camera.view;//testCamera.view;



        {
            PROFILE_SCOPE_TAB("Camera Position Chunk Update");

#ifdef _DEBUG
            g_camera.drawDistance = 10;
#elif NDEBUG
            g_camera.drawDistance = 20;//60;
#endif

            g_camera.fogDistance = g_camera.drawDistance + 10;
            ChunkPos cam = ToChunk(g_camera.transform.m_p);
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
                PROFILE_SCOPE_TAB("Chunk Delete Check");
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

        RenderUpdate(g_window.size, deltaTime);

        //SKYBOX
        {
            //glBlendFunc(GL_ONE, GL_ONE);
            PROFILE_SCOPE_TAB("Draw Skybox");

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
            sp->UpdateUniformVec3("u_cameraPosition", 1, g_camera.transform.m_p.p.e);
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
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        {
            PROFILE_SCOPE_TAB("Semaphore Update");

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
            PROFILE_SCOPE_TAB("Chunk Loading Vertex Loop");

            for (int32 _drawDistance = 0; _drawDistance < g_camera.drawDistance; _drawDistance++)
            {
                for (int32 drawZ = -_drawDistance; drawZ <= _drawDistance; drawZ++)
                {
                    for (int32 drawX = -_drawDistance; drawX <= _drawDistance; drawX++)
                    {
                        if ((drawX < _drawDistance && drawX > -_drawDistance) &&
                            (drawZ < _drawDistance && drawZ > -_drawDistance))
                            continue;

                        ChunkPos cameraChunkP = ToChunk(g_camera.transform.m_p);
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
            PROFILE_SCOPE_TAB("Chunk Upload and Render");

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
                    renderables[numRenderables].d = ManhattanDistance(ToChunk(g_camera.transform.m_p.p).p, chunkP.p);
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
            PROFILE_SCOPE_TAB("Debug Code");
            //DrawBlock(testCamera.p.p, { 0, 1, 0, 1 }, 5.0f, perspective);
            //DrawBlock(lookatPosition, { 1, 0, 0, 1 }, 5.0f, perspective);
            if (s_debugFlags & +DebugOptions::Enabled)
            {
                if (s_debugFlags & +DebugOptions::ChunkStatus)
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
                        chunkP.p.y = float(g_chunks->height[i] + 1);
                        //chunkP.p.y = CHUNK_Y;
                        chunkP.p.z += CHUNK_Z / 2.0f;
                        Vec3 size = { CHUNK_X / 4.0f, 1, CHUNK_Z / 4.0f };

                        DrawBlock(chunkP, colors[static_cast<int32>(g_chunks->state[i])], size, perspective);
                    }
                }
                for (WorldPos p : cubesToDraw)
                {
                    DrawBlock(p, Red, 2.0f, perspective);
                }
                if (s_debugFlags & +DebugOptions::LookatBlock)
                {
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
                if (s_debugFlags & +DebugOptions::CollisionTriangles)
                {
                    if (debug_trianglesToDraw.size())
                        DrawTriangles(debug_trianglesToDraw, Orange, perspective, false);
                }
            }
        }

        {
            PROFILE_SCOPE("Chunk Deletion");
            for (ChunkIndex i = 0; i < g_chunks->highestActiveChunk; i++)
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

        {
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
            left.botLeft  = { screenSizeHalf.x - halfLineBounds,  center.botLeft.y };
            left.topRight = { center.botLeft.x, center.topRight.y };

            RectInt top;
            top.botLeft  = left.topRight;
            top.topRight = { center.topRight.x, screenSizeHalf.y + halfLineBounds };

            RectInt right;
            right.botLeft  = { center.topRight.x, center.botLeft.y };
            right.topRight = { screenSizeHalf.x + halfLineBounds,    center.topRight.y };

            RectInt bot;
            bot.botLeft  = { center.botLeft.x,  screenSizeHalf.y - halfLineBounds};
            bot.topRight = right.botLeft;

            UI_AddDrawCall({}, center, crosshairColor, Texture::T::Plain);
            UI_AddDrawCall({}, left,   crosshairColor, Texture::T::Plain);
            UI_AddDrawCall({}, top,    crosshairColor, Texture::T::Plain);
            UI_AddDrawCall({}, right,  crosshairColor, Texture::T::Plain);
            UI_AddDrawCall({}, bot,    crosshairColor, Texture::T::Plain);

            UI_Render();
        }

        ResolveMSAAFramebuffer();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, g_window.size.x, g_window.size.y);
        g_renderer.postTarget->m_color->Bind();
        g_renderer.programs[+Shader::BufferCopy]->UseShader();
        g_renderer.postVertexBuffer->Bind();

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
        glEnableVertexArrayAttrib(g_renderer.vao, 2);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


        ImGui::Render();
        //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        //glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (g_cursorEngaged)
            SDL_ShowCursor(SDL_DISABLE);

        SDL_GL_SwapWindow(g_window.SDL_Context);
        glEnable(GL_DEPTH_TEST);
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
