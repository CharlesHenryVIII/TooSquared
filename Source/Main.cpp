#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
//#define _DEBUGPRINT
//#define _2DRENDERING
#include "SDL/include/SDL.h"
#include "Math.h"
#include "glew.h"
#include "STB/stb_image.h"
#include "Misc.h"
#include "Rendering.h"

#include <Windows.h>
#include <unordered_map>

struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context = nullptr;
}g_window;

struct Key {
	bool down;
	bool downPrevFrame;
	bool downThisFrame;
	bool upThisFrame;
};

struct Mouse {
	Vec2Int pos;
	Vec2Int wheel; //Y for vertical rotations, X for Horizontal rotations/movement
}g_mouse;

Vec3 cameraPosition = { 2, 2, 2 };



#ifdef _2DRENDERING
class VertexBuffer2 : public GpuBuffer
{
public:
    VertexBuffer2()
        : GpuBuffer(GL_ARRAY_BUFFER)
    { }

    void Upload(Vert2d* vertices, size_t count)
    {
        UploadData(vertices, sizeof(vertices[0]) * count);
    }
};
#endif

struct Renderer {
    SDL_GLContext GL_Context = {};
    ShaderProgram* programs[+Shader::Count] = {};
    Texture* textures[Texture::Count] = {};
    IndexBuffer* squareIndexBuffer = nullptr;
    GLuint vao;
}g_renderer;

#ifdef _2DRENDERING
uint32 squareIndexes[] = {
	0,1,2,1,2,3,
};
#endif
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

float p = 1.0f;
float tf = 1.0f;
float te = 0.0f;
Vertex cubeVertices[] = {
    { { -p,  p, -p }, { tf, te }, { -1.0f, 0.0f, 0.0f } }, // top right
    { { -p, -p, -p }, { te, te }, { -1.0f, 0.0f, 0.0f } }, // Top Left
    { { -p,  p,  p }, { tf, tf }, { -1.0f, 0.0f, 0.0f } }, // bot right
    { { -p, -p,  p }, { te, tf }, { -1.0f, 0.0f, 0.0f } }, // -x bottom Left

    { {  p,  p, -p }, { tf, te }, {  1.0f, 0.0f, 0.0f } },
    { {  p, -p, -p }, { te, te }, {  1.0f, 0.0f, 0.0f } },
    { {  p,  p,  p }, { tf, tf }, {  1.0f, 0.0f, 0.0f } },
    { {  p, -p,  p }, { te, tf }, {  1.0f, 0.0f, 0.0f } }, // +x

    { { -p, -p,  p }, { te, tf }, { 0.0f, -1.0f, 0.0f } }, // -y
    { { -p, -p, -p }, { te, te }, { 0.0f, -1.0f, 0.0f } },
    { {  p, -p,  p }, { tf, tf }, { 0.0f, -1.0f, 0.0f } },
    { {  p, -p, -p }, { tf, te }, { 0.0f, -1.0f, 0.0f } },

    { { -p,  p,  p }, { te, tf }, { 0.0f,  1.0f, 0.0f } }, // +y
    { { -p,  p, -p }, { te, te }, { 0.0f,  1.0f, 0.0f } },
    { {  p,  p,  p }, { tf, tf }, { 0.0f,  1.0f, 0.0f } },
    { {  p,  p, -p }, { tf, te }, { 0.0f,  1.0f, 0.0f } },

    { { -p,  p, -p }, { te, tf }, { 0.0f, 0.0f, -1.0f } }, // -z
    { { -p, -p, -p }, { te, te }, { 0.0f, 0.0f, -1.0f } },
    { {  p,  p, -p }, { tf, tf }, { 0.0f, 0.0f, -1.0f } },
    { {  p, -p, -p }, { tf, te }, { 0.0f, 0.0f, -1.0f } },

    { { -p,  p,  p }, { te, tf }, { 0.0f, 0.0f,  1.0f } }, // z
    { { -p, -p,  p }, { te, te }, { 0.0f, 0.0f,  1.0f } },
    { {  p,  p,  p }, { tf, tf }, { 0.0f, 0.0f,  1.0f } },
    { {  p, -p,  p }, { tf, te }, { 0.0f, 0.0f,  1.0f } },
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

		gbMat4 perspective;
		gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
		gbMat4 view;
		Vec3 a = { 1.0f, 1.0f, 1.0f };
		gb_mat4_look_at(&view, cameraPosition + a, cameraPosition, { 0,1,0 });
        gbMat4 transform;
		gb_mat4_identity(&transform);

        ShaderProgram* p = g_renderer.programs[+Shader::Simple3D];
        p->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
        p->UpdateUniformMat4("u_view", 1, false, view.e);
        p->UpdateUniformMat4("u_model", 1, false, transform.e);

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




const char* pix3DShaderText = R"term(
)term";

#ifdef _2DRENDERING
const char* vertexShaderText = R"term(
#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;

out vec2 p_uv;
out vec3 p_normal;

void main()
{
    p_uv = v_uv;
    gl_Position = vec4(v_position, 1.0);
    //p_normal = vec3(0,0,0);
}
)term";

const char* pixelShaderText = R"term(
#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;

out vec4 color;

void main()
{

    color = texture(sampler, p_uv);

}
)term";
#endif


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


#ifdef _2DRENDERING
	//g_renderer.squareIndexBuffer = new IndexBuffer();
	//g_renderer.squareIndexBuffer->Upload(squareIndexes, arrsize(squareIndexes));
#endif

	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215.png");
#ifdef _2DRENDERING
	//g_renderer.programs[+Shader::Simple2D] = new ShaderProgram(vertexShaderText, pixelShaderText);
#endif
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

#ifdef _2DRENDERING
void Draw2DTexture(Texture t, Rect s, Rect d)
{
	glDepthMask(GL_FALSE);
	float iblx = Clamp(s.botLeft.x  / t.size.x, 0.0f, 1.0f);
	float ibly = Clamp(s.botLeft.y  / t.size.y, 0.0f, 1.0f);
	float itrx = Clamp(s.topRight.x / t.size.x, 0.0f, 1.0f);
	float itry = Clamp(s.topRight.y / t.size.y, 0.0f, 1.0f);

    float dblx = Clamp(d.botLeft.x,  -1.0f, 1.0f);
    float dbly = Clamp(d.botLeft.y,  -1.0f, 1.0f);
    float dtrx = Clamp(d.topRight.x, -1.0f, 1.0f);
    float dtry = Clamp(d.topRight.y, -1.0f, 1.0f);

    Vert3d vertices[] = {
        { { dblx, dbly, 0}, { iblx, ibly }, {} }, //bot left
        { { dblx, dtry, 0}, { iblx, itry }, {} }, //top left
        { { dtrx, dbly, 0}, { itrx, ibly }, {} }, //bot right
        { { dtrx, dtry, 0}, { itrx, itry }, {} }, //top right
    };
    //NOTE: Just for reference if the vertices are messed up:
    //Vertex vertices[] = {
    //	{ -1, -1, 0, 0.0f, 0.0f },
    //	{ -1,  1, 0, 0.0f, 1.0f },
    //	{  1, -1, 0, 1.0f, 0.0f },
    //	{  1,  1, 0, 1.0f, 1.0f },
    //};

    VertexBuffer3 vertexBuffer;
    vertexBuffer.Upload(vertices, sizeof(vertices));
    vertexBuffer.Bind();

    //GLint size = 0;
    //glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    //if (size != sizeof(vertices))
    //{
    //    DebugPrint("ERROR");
    //    FAIL;
    //}
    //glUseProgram(g_renderer.programs[+program]);


    g_renderer.squareIndexBuffer->Bind();
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_renderer.squareIndexBuffer);
    glBindTexture(GL_TEXTURE_2D, t.gl_handle);

    g_renderer.programs[+Shader::Simple2D]->UseShader();
    //glEnableVertexArrayAttrib(g_renderer.vao, 0);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert2d), (void*)offsetof(Vert2d, x));
    //glEnableVertexArrayAttrib(g_renderer.vao, 1);
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vert2d), (void*)offsetof(Vert2d, u));
    //glDisableVertexArrayAttrib(g_renderer.vao, 2);

    glDrawElements(GL_TRIANGLES, sizeof(squareIndexes) / sizeof(uint32), GL_UNSIGNED_INT, 0);
}
#endif

double s_lastShaderUpdateTime = 0;
double s_incrimentalTime = 0;
void RenderUpdate(float deltaTime)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (s_lastShaderUpdateTime + 0.1f <= s_incrimentalTime)
	{
		for (ShaderProgram* s : g_renderer.programs)
		{
			if (s)
				s->CheckForUpdate();
		}
		s_lastShaderUpdateTime = s_incrimentalTime;
	}
    s_incrimentalTime += deltaTime;


#ifdef _2DRENDERING
	Rect imageSource = {
		.botLeft = { 0 ,  1440 },
		.topRight = { 16 , 1456 },
	};
	Rect screenLocation = {
		.botLeft = { -1, -1 },
		.topRight = {  1,  1 },
	};
	//Draw2DTexture(*g_renderer.textures[Texture::Minecraft], GetRectFromSprite(0), screenLocation);
#endif

}

int main(int argc, char* argv[])
{
    std::unordered_map<int32, Key> keyStates;

    InitializeVideo();

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    Grass* testGrassBlock = new Grass();

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
                int mods = 0;
				keyStates[SDLEvent.key.keysym.sym].down = true;
                break;
            }
            case SDL_KEYUP:
                keyStates[SDLEvent.key.keysym.sym].down = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
				keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                break;
            case SDL_MOUSEMOTION:
                g_mouse.pos.x = SDLEvent.motion.x;
                g_mouse.pos.y = SDLEvent.motion.y;
                break;
            case SDL_MOUSEWHEEL:
            {
                g_mouse.wheel.x = SDLEvent.wheel.x;
                g_mouse.wheel.y = SDLEvent.wheel.y;
                break;
            }
            case SDL_WINDOWEVENT:
                if (SDLEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    g_window.size.x = SDLEvent.window.data1;
                    g_window.size.y = SDLEvent.window.data2;
                    glViewport(0, 0, g_window.size.x, g_window.size.y);
                }
                else if (SDLEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                {
                    SDL_CaptureMouse(SDL_TRUE);
                }
                break;
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


        const float cameraMovement = 0.5f;

        if (keyStates[SDLK_s].down)
            cameraPosition.z += cameraMovement * deltaTime;
        if (keyStates[SDLK_w].down)
            cameraPosition.z -= cameraMovement * deltaTime;
        if (keyStates[SDLK_d].down)
            cameraPosition.x += cameraMovement * deltaTime;
        if (keyStates[SDLK_a].down)
            cameraPosition.x -= cameraMovement * deltaTime;
        if (keyStates[SDLK_q].down)
            cameraPosition.y += cameraMovement * deltaTime;
        if (keyStates[SDLK_e].down)
            cameraPosition.y -= cameraMovement * deltaTime;
        if (keyStates[SDLK_BACKQUOTE].down)
            g_running = false;


        RenderUpdate(deltaTime);
		testGrassBlock->Render();

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
