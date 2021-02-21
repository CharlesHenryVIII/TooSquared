#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define _DEBUGPRINT
#include "SDL/include/SDL.h"
#include "Math.h"
#include "glew.h"
#include "STB/stb_image.h"
#include "Misc.h"
#include "Rendering.h"

#include <Windows.h>
#include <unordered_map>

struct Key {
	bool down;
	bool downPrevFrame;
	bool downThisFrame;
	bool upThisFrame;
};

struct Mouse {
	Vec2Int pos = {};
    Vec2Int pDelta = {};
	Vec2Int wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
}g_mouse;

struct Camera {
    Vec3 p = { 2, 2, 2 };
    Vec3 r = {};
    Mat4 view;
} g_camera;

uint32 cubeIndices[36] = {};

enum class BlockType : uint32 {
    Invalid,
    Grass,
    Stone,
    Planks,
    StoneSlab,
    Brick,
    TNT,
    CobWeb,
    Flower_Red,
    Flower_Yellow,
    Flower_Blue,
    Sappling,
    Cobblestone,
    Bedrock,
    Sand,
    Gravel,
    Wood,
    Count,
    Iron_Block,
    Gold_Block,
    Diamond_Block,
    Chest,
};
ENUMOPS(BlockType);

enum class Face : uint32 {
	Front,
	Back,
	Bot,
	Top,
	Right,
	Left,
	Count,
};
ENUMOPS(Face);

const uint32 pixelsPerBlock = 16;
const uint32 blocksPerRow = 16;
Rect GetRectFromSprite(uint32 i)
{
    assert(i <= blocksPerRow * blocksPerRow);
    i = Clamp<uint32>(i, 0, blocksPerRow);
    uint32 x = i % blocksPerRow;
    uint32 y = i / blocksPerRow;

    Rect result = {};
    result.botLeft.x = static_cast<float>(x * pixelsPerBlock);
    result.botLeft.y = static_cast<float>(pixelsPerBlock * (blocksPerRow - y - 1));
    result.topRight.x = result.botLeft.x + pixelsPerBlock;
    result.topRight.y = result.botLeft.y + pixelsPerBlock;
    return result;
}

Vertex cubeVertices[] = {
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // top right
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // Top Left
    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // bot right
    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // -x bottom Left

    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } }, // +x

    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // -y
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } }, // +y
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },

    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // -z
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } }, // z
    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
};

struct Block {
    Vec3 p = {};
    BlockType t = BlockType::Invalid;
    uint32 defaultSpriteLocation = 254;
    uint32 spriteLocation[static_cast<uint32>(Face::Count)] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

	void Render()
	{
		for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
		{
            uint32 tileIndex = spriteLocation[faceIndex];
            if (tileIndex == 254)
                tileIndex = defaultSpriteLocation;
			Rect s = GetRectFromSprite(tileIndex);

            const uint32 size = blocksPerRow * pixelsPerBlock;
			float iblx = Clamp(s.botLeft.x  / size, 0.0f, 1.0f);
			float ibly = Clamp(s.botLeft.y  / size, 0.0f, 1.0f);
			float itrx = Clamp(s.topRight.x / size, 0.0f, 1.0f);
			float itry = Clamp(s.topRight.y / size, 0.0f, 1.0f);

			cubeVertices[faceIndex * 4 + 0].uv = { iblx, itry }; //Bot Left
			cubeVertices[faceIndex * 4 + 1].uv = { iblx, ibly }; //Top Left
			cubeVertices[faceIndex * 4 + 2].uv = { itrx, itry }; //Bot Right
			cubeVertices[faceIndex * 4 + 3].uv = { itrx, ibly }; //Top Right
		}
        vertexBuffer.Upload(cubeVertices, arrsize(cubeVertices));
        indexBuffer.Upload(cubeIndices,  arrsize(cubeIndices));
        g_renderer.textures[Texture::Minecraft]->Bind();
        g_renderer.programs[+Shader::Simple3D]->UseShader();

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
		glEnableVertexArrayAttrib(g_renderer.vao, 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
		glEnableVertexArrayAttrib(g_renderer.vao, 1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
		glEnableVertexArrayAttrib(g_renderer.vao, 2);

		Mat4 perspective;
		gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
        Mat4 transform;
        gb_mat4_translate(&transform, p);

        ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
        sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, g_camera.view.e);
        sp->UpdateUniformMat4("u_model", 1, false, transform.e);

		glDrawElements(GL_TRIANGLES, arrsize(cubeIndices), GL_UNSIGNED_INT, 0);
	}
};

struct Grass : public Block {

    Grass()
    {
        defaultSpriteLocation = 3;
        spriteLocation[+Face::Top] = 0;
        spriteLocation[+Face::Bot] = 2;
    }
};

void OpenGLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                         GLsizei length, const GLchar *message, const void *userParam)
{
    std::string _severity;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;
        case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;
        case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        return;
        //break;
        default:
        _severity = "UNKNOWN";
        break;
    }

    DebugPrint("%s severity: %s\n",
            _severity.c_str(), message);
    //DebugPrint("%d: %s of %s severity, raised from %s: \n%s\n",
    //        id, _type.c_str(), _severity.c_str(), _source.c_str(), message);
    //DebugPrint("vs \n");
    //DebugPrint("%s\n\n", message);

    AssertOnce(severity != GL_DEBUG_SEVERITY_HIGH);
}

void InitializeVideo()
{
    SDL_Init(SDL_INIT_VIDEO);
    {
        SDL_Rect screenSize = {};
        SDL_GetDisplayBounds(0, &screenSize);
        float displayRatio = 16 / 9.0f;
        g_window.size.x = screenSize.w / 2;
        g_window.size.y = Clamp<int>(int(g_window.size.x / displayRatio), 50, screenSize.h);
        g_window.pos.x = g_window.size.x / 2;
        g_window.pos.y = g_window.size.y / 2;
    }

    uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    g_window.SDL_Context = SDL_CreateWindow("TooSquared", g_window.pos.x, g_window.pos.y, g_window.size.x, g_window.size.y, windowFlags);
    /* Create an OpenGL context associated with the window. */

    {
        const int32 majorVersionRequest = 4;//3;
        const int32 minorVersionRequest = 3;//2;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        g_renderer.GL_Context = SDL_GL_CreateContext(g_window.SDL_Context);
        SDL_GL_MakeCurrent(g_window.SDL_Context, g_renderer.GL_Context);
        GLint majorVersionActual = {};
        GLint minorVersionActual = {};
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersionActual);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersionActual);

        if (majorVersionRequest != majorVersionActual && minorVersionRequest != minorVersionActual)
        {
            DebugPrint("OpenGL could not set recommended version: %i.%i to %i.%i\n",
                    majorVersionRequest, minorVersionRequest,
                    majorVersionActual,  minorVersionActual);
            FAIL;
        }
    }

    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(1);
    glewExperimental = true;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        DebugPrint("Error: %s\n", glewGetErrorString(err));
    }
    DebugPrint("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	stbi_set_flip_vertically_on_load(true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLErrorCallback, NULL);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_FALSE);

	static_assert(arrsize(cubeVertices) == 24, "");

	glGenVertexArrays(1, &g_renderer.vao);
	glBindVertexArray(g_renderer.vao);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215.png");
	g_renderer.programs[+Shader::Simple3D] = new ShaderProgram("Source/Shaders/3D.vert", "Source/Shaders/3D.frag");

	for (int face = 0; face < 6; ++face)
	{
		int base_index = face * 4;
		cubeIndices[face * 6 + 0] = base_index + 0;
		cubeIndices[face * 6 + 1] = base_index + 1;
		cubeIndices[face * 6 + 2] = base_index + 2;
		cubeIndices[face * 6 + 3] = base_index + 1;
		cubeIndices[face * 6 + 4] = base_index + 2;
		cubeIndices[face * 6 + 5] = base_index + 3;
	}
}

int main(int argc, char* argv[])
{
    std::unordered_map<int32, Key> keyStates;

    InitializeVideo();

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    g_camera.view;
	Vec3 a = { 1.0f, 1.0f, 1.0f };
	gb_mat4_look_at(&g_camera.view, g_camera.p + a, g_camera.p, { 0,1,0 });

	std::vector<Grass*> grassBlockList;
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
					grassBlockList.push_back(temp);
				}
			}
		}
		{
			Grass* temp = new Grass();
			temp->p = { 0.0f, 1.0f, 0.0f };
			grassBlockList.push_back(temp);
		}
	}

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
					g_mouse.pDelta.x = SDLEvent.motion.x - g_mouse.pos.x;
					g_mouse.pos.x = SDLEvent.motion.x;
					g_mouse.pDelta.y = SDLEvent.motion.y - g_mouse.pos.y;
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


        RenderUpdate(deltaTime);
        for (Grass* g : grassBlockList)
        {
            if (g)
                g->Render();
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
