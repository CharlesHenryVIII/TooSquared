#include "SDL/include/SDL.h"
#include "Math.h"
#include "glew.h"
#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"

#include <Windows.h>
#include <unordered_map>
#include <type_traits>

bool g_running = true;

struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context;
}g_window;

#define FAIL assert(false);
#define arrsize(arr__) (sizeof(arr__) / sizeof(arr__[0]))

//#define Enum(name) enum class name; \
//inline uint32 operator+(name a)     \
//{                                   \
//	return static_cast<uint32>(a);  \
//}                                   \
//enum class name

#define ENUMOPS(T) \
constexpr auto operator+(T a)\
{                       \
    return static_cast<typename std::underlying_type<T>::type>(a);\
}

enum class Shader : uint32 {
    Invalid,
    Simple2D,
    Simple3D,
    Count,
};
ENUMOPS(Shader);

struct Rect {
    Vec2 botLeft = {};
    Vec2 topRight = {};
};

struct Texture {
    Vec2Int size = {};
    int32 n = 0;//bytes per pixel
    uint8* data = {};
    GLuint GL_Handle = {};
};

struct DrawCall {
    //Texture* texture = nullptr;
    GLuint texture = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    Shader program = Shader::Invalid;
    gbMat4 transform;
};

struct Renderer {
    SDL_GLContext GL_Context = {};
    GLuint programs[+(Shader::Count)];
	std::vector<DrawCall> drawcalls;
	std::unordered_map<std::string, Texture> textures;
	GLuint squareIndexBuffer;
    GLuint vao;
}g_renderer;

uint32 squareIndexes[] = {
	0,1,2,1,2,3,
};
int32 indices3D[36] = {};

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


void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    va_end(list);
}

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
	Top,
	Bot,
	Left,
	Right,
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

struct Block {
    Vec3 p = {};
    BlockType t = BlockType::Invalid;
    uint32 defaultSpriteLocation = 254;
    uint32 spriteLocation[static_cast<uint32>(Face::Count)] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };

	void Render()
	{
		DrawCall dc = {};

		float p = 1.0f;
		float tf = 1.0f;
		float te = 0.0f;
		Vert3d cubeVertices[] = {
			{ { -p, -p,  p }, { -1.0f, 0.0f, 0.0f }, { te, tf } }, // -x bottom Left
			{ { -p, -p, -p }, { -1.0f, 0.0f, 0.0f }, { te, te } }, // Top Left
			{ { -p,  p,  p }, { -1.0f, 0.0f, 0.0f }, { tf, tf } }, // bot right
			{ { -p,  p, -p }, { -1.0f, 0.0f, 0.0f }, { tf, te } }, // top right

			{ {  p, -p,  p }, { 1.0f, 0.0f, 0.0f }, { te, tf } }, // +x
			{ {  p, -p, -p }, { 1.0f, 0.0f, 0.0f }, { te, te } },
			{ {  p,  p,  p }, { 1.0f, 0.0f, 0.0f }, { tf, tf } },
			{ {  p,  p, -p }, { 1.0f, 0.0f, 0.0f }, { tf, te } },

			{ { -p, -p,  p }, { 0.0f, -1.0f, 0.0f }, { te, tf } }, // -y
			{ { -p, -p, -p }, { 0.0f, -1.0f, 0.0f }, { te, te } },
			{ {  p, -p,  p }, { 0.0f, -1.0f, 0.0f }, { tf, tf } },
			{ {  p, -p, -p }, { 0.0f, -1.0f, 0.0f }, { tf, te } },

			{ { -p,  p,  p }, { 0.0f, 1.0f, 0.0f }, { te, tf } }, // +y
			{ { -p,  p, -p }, { 0.0f, 1.0f, 0.0f }, { te, te } },
			{ {  p,  p,  p }, { 0.0f, 1.0f, 0.0f }, { tf, tf } },
			{ {  p,  p, -p }, { 0.0f, 1.0f, 0.0f }, { tf, te } },

			{ { -p,  p, -p }, { 0.0f, 0.0f, -1.0f }, { te, tf } }, // -z
			{ { -p, -p, -p }, { 0.0f, 0.0f, -1.0f }, { te, te } },
			{ {  p,  p, -p }, { 0.0f, 0.0f, -1.0f }, { tf, tf } },
			{ {  p, -p, -p }, { 0.0f, 0.0f, -1.0f }, { tf, te } },

			{ { -p,  p,  p }, { 0.0f, 0.0f, 1.0f }, { te, tf } }, // z
			{ { -p, -p,  p }, { 0.0f, 0.0f, 1.0f }, { te, te } },
			{ {  p,  p,  p }, { 0.0f, 0.0f, 1.0f }, { tf, tf } },
			{ {  p, -p,  p }, { 0.0f, 0.0f, 1.0f }, { tf, te } },
		};
		static_assert(arrsize(cubeVertices) == 24, "");

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

		glGenBuffers(1, &dc.vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, dc.vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

		glGenBuffers(1, &dc.indexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices3D), indices3D, GL_STATIC_DRAW);

		gb_mat4_identity(&dc.transform);
		//dc.transform = ;

		dc.texture = g_renderer.textures["MinecraftSpriteSheet"].GL_Handle;
		dc.program = Shader::Simple3D;
		g_renderer.drawcalls.push_back(dc);
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

GLuint CreateShaderProgram(const char* vertexText, const char* pixelText)
{
	GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	GLuint pixShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShaderHandle, 1, &vertexText, 0);
	glCompileShader(vertexShaderHandle);

	GLint status;
	glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint log_length;
		glGetShaderiv(vertexShaderHandle, GL_INFO_LOG_LENGTH, &log_length);
		GLchar info[4096];
		glGetShaderInfoLog(vertexShaderHandle, log_length, NULL, info);
		DebugPrint("Vertex Shader compilation error: %s\n", info);
        FAIL;
	}

	glShaderSource(pixShaderHandle, 1, &pixelText, 0);
	glCompileShader(pixShaderHandle);

	glGetShaderiv(pixShaderHandle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint log_length;
		glGetShaderiv(pixShaderHandle, GL_INFO_LOG_LENGTH, &log_length);
		GLchar info[4096];
		glGetShaderInfoLog(pixShaderHandle, log_length, NULL, info);
		DebugPrint("Shader compilation error: %s\n", info);
        FAIL;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShaderHandle);
	glAttachShader(program, pixShaderHandle);
	glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        GLchar info[4096];
        glGetProgramInfoLog(program, log_length, NULL, info);
        DebugPrint("Shader linking error: %s\n", info);
        FAIL;
    }
	return program;
}

const char* vert3DShaderText = R"term(
#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 p_uv;
out vec3 p_normal;

void main()
{
    p_uv = v_uv;
    //p_normal = (u_view * u_model * vec4(v_normal, 0)).xyz;
    p_normal = v_normal;
	gl_Position = u_perspective * u_view * u_model * vec4(v_position, 1.0);
}
)term";

const char* pix3DShaderText = R"term(
#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;

out vec4 color;

void main()
{
    color = texture(sampler, p_uv);
    //color.xyz = p_normal;
    //color.a = 1;
}
)term";

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
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    // Enable the debug callback
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLErrorCallback, NULL);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_FALSE);
}

Texture CreateTexture(const char* c)
{
    Texture result = {};
	result.data = stbi_load(c, &result.size.x, &result.size.y, &result.n, STBI_rgb_alpha);

	glGenTextures(1, &result.GL_Handle);
	glBindTexture(GL_TEXTURE_2D, result.GL_Handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result.size.x, result.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, result.data);

    return result;
}

//NOTE: DrawCall screenLocation is based on the -1 to +1 locations
DrawCall CreateDrawCall(Rect s, Rect d, Texture* t, Shader program)
{
    DrawCall dc = {};

	float iblx = Clamp(s.botLeft.x  / t->size.x, 0.0f, 1.0f);
	float ibly = Clamp(s.botLeft.y  / t->size.y, 0.0f, 1.0f);
	float itrx = Clamp(s.topRight.x / t->size.x, 0.0f, 1.0f);
	float itry = Clamp(s.topRight.y / t->size.y, 0.0f, 1.0f);

	switch (program)
    {
    case Shader::Simple2D:
	{

		float dblx = Clamp(d.botLeft.x,  -1.0f, 1.0f);
		float dbly = Clamp(d.botLeft.y,  -1.0f, 1.0f);
		float dtrx = Clamp(d.topRight.x, -1.0f, 1.0f);
		float dtry = Clamp(d.topRight.y, -1.0f, 1.0f);

		Vert2d vertices[] = {
			{ dblx, dbly, 0, iblx, ibly }, //bot left
			{ dblx, dtry, 0, iblx, itry }, //top left
			{ dtrx, dbly, 0, itrx, ibly }, //bot right
			{ dtrx, dtry, 0, itrx, itry }, //top right
		};
		//NOTE: Just for reference if the vertices are messed up:
		//Vertex vertices[] = {
		//	{ -1, -1, 0, 0.0f, 0.0f },
		//	{ -1,  1, 0, 0.0f, 1.0f },
		//	{  1, -1, 0, 1.0f, 0.0f },
		//	{  1,  1, 0, 1.0f, 1.0f },
		//};

		GLuint vertexBuffer;
		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		GLint size = 0;
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		if (size != sizeof(vertices))
		{
			DebugPrint("ERROR");
			FAIL;
		}
		//glUseProgram(g_renderer.programs[+program]);

		dc.vertexBuffer = vertexBuffer;
		dc.indexBuffer = g_renderer.squareIndexBuffer;
		break;
	}
	case Shader::Simple3D:
	{
        float p = 1.0f;
        float tf = 1.0f;
        float te = 0.0f;

        s.botLeft.x;

        Vert3d cubeVertices[] = {
            { { -p, -p,  p }, { -1.0f, 0.0f, 0.0f }, { te, tf } }, // -x
            { { -p, -p, -p }, { -1.0f, 0.0f, 0.0f }, { te, te } },
            { { -p,  p,  p }, { -1.0f, 0.0f, 0.0f }, { tf, tf } },
            { { -p,  p, -p }, { -1.0f, 0.0f, 0.0f }, { tf, te } },

            { {  p, -p,  p }, { 1.0f, 0.0f, 0.0f }, { te, tf } }, // +x
            { {  p, -p, -p }, { 1.0f, 0.0f, 0.0f }, { te, te } },
            { {  p,  p,  p }, { 1.0f, 0.0f, 0.0f }, { tf, tf } },
            { {  p,  p, -p }, { 1.0f, 0.0f, 0.0f }, { tf, te } },

            { { -p, -p,  p }, { 0.0f, -1.0f, 0.0f }, { te, tf } }, // -y
            { { -p, -p, -p }, { 0.0f, -1.0f, 0.0f }, { te, te } },
            { {  p, -p,  p }, { 0.0f, -1.0f, 0.0f }, { tf, tf } },
            { {  p, -p, -p }, { 0.0f, -1.0f, 0.0f }, { tf, te } },

            { { -p,  p,  p }, { 0.0f, 1.0f, 0.0f }, { te, tf } }, // +y
            { { -p,  p, -p }, { 0.0f, 1.0f, 0.0f }, { te, te } },
            { {  p,  p,  p }, { 0.0f, 1.0f, 0.0f }, { tf, tf } },
            { {  p,  p, -p }, { 0.0f, 1.0f, 0.0f }, { tf, te } },

            { { -p,  p, -p }, { 0.0f, 0.0f, -1.0f }, { te, tf } }, // -z
            { { -p, -p, -p }, { 0.0f, 0.0f, -1.0f }, { te, te } },
            { {  p,  p, -p }, { 0.0f, 0.0f, -1.0f }, { tf, tf } },
            { {  p, -p, -p }, { 0.0f, 0.0f, -1.0f }, { tf, te } },

            { { -p,  p,  p }, { 0.0f, 0.0f, 1.0f }, { te, tf } }, // z
            { { -p, -p,  p }, { 0.0f, 0.0f, 1.0f }, { te, te } },
            { {  p,  p,  p }, { 0.0f, 0.0f, 1.0f }, { tf, tf } },
            { {  p, -p,  p }, { 0.0f, 0.0f, 1.0f }, { tf, te } },
        };
        static_assert(arrsize(cubeVertices) == 24, "");


        glGenBuffers(1, &dc.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, dc.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &dc.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices3D), indices3D, GL_STATIC_DRAW);

        gb_mat4_identity(&dc.transform);
        //dc.transform = ;

        break;
    }
    }

    dc.texture = t->GL_Handle;
    dc.program = program;
    return dc;
}

Vec3 cameraPosition = { 2, 2, 2 };
void RenderUpdate()
{
    //glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (DrawCall& dc : g_renderer.drawcalls)
    {

        glBindBuffer(GL_ARRAY_BUFFER, dc.vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc.indexBuffer);
        glBindTexture(GL_TEXTURE_2D, dc.texture);
        glUseProgram(g_renderer.programs[+dc.program]);

        switch (dc.program)
        {
        case Shader::Simple2D:
		{
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            glEnableVertexArrayAttrib(g_renderer.vao, 0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert2d), (void*)offsetof(Vert2d, x));
            glEnableVertexArrayAttrib(g_renderer.vao, 1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vert2d), (void*)offsetof(Vert2d, u));
            glDisableVertexArrayAttrib(g_renderer.vao, 2);

            glDrawElements(GL_TRIANGLES, sizeof(squareIndexes) / sizeof(uint32), GL_UNSIGNED_INT, 0);
			break;
		}
		case Shader::Simple3D:
		{
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);

            glEnableVertexArrayAttrib(g_renderer.vao, 0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert3d), (void*)offsetof(Vert3d, p));
            glEnableVertexArrayAttrib(g_renderer.vao, 1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vert3d), (void*)offsetof(Vert3d, n));
            glEnableVertexArrayAttrib(g_renderer.vao, 2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vert3d), (void*)offsetof(Vert3d, uv));

            gbMat4 perspective;
            gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
            gbMat4 view;
            Vec3 a = { 1.0f, 1.0f, 1.0f };
            gb_mat4_look_at(&view, cameraPosition + a, cameraPosition, { 0,1,0 });


            GLint loc;
            loc = glGetUniformLocation(g_renderer.programs[+dc.program], "u_perspective");
            glUniformMatrix4fv(loc, 1, false, perspective.e);

            loc = glGetUniformLocation(g_renderer.programs[+dc.program], "u_view");
            glUniformMatrix4fv(loc, 1, false, view.e);

            loc = glGetUniformLocation(g_renderer.programs[+dc.program], "u_model");
            glUniformMatrix4fv(loc, 1, false, dc.transform.e);

            glDrawElements(GL_TRIANGLES, arrsize(indices3D), GL_UNSIGNED_INT, 0);
			break;
		}
        default:
        {
            FAIL;
            break;
        }
        }

    }

    g_renderer.drawcalls.clear();
}

int main(int argc, char* argv[])
{
    std::unordered_map<int32, Key> keyStates;

    InitializeVideo();

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	double previousTime = totalTime;

	glGenBuffers(1, &g_renderer.squareIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_renderer.squareIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndexes), squareIndexes, GL_STATIC_DRAW);

    g_renderer.textures["MinecraftSpriteSheet"] = CreateTexture("Assets/MinecraftSpriteSheet20120215.png");
    g_renderer.programs[+Shader::Simple2D] = CreateShaderProgram(vertexShaderText, pixelShaderText);
    g_renderer.programs[+Shader::Simple3D] = CreateShaderProgram(vert3DShaderText, pix3DShaderText);

    for (int face = 0; face < 6; ++face)
    {
        int base_index = face * 4;
        indices3D[face * 6 + 0] = base_index + 0;
        indices3D[face * 6 + 1] = base_index + 1;
        indices3D[face * 6 + 2] = base_index + 2;
        indices3D[face * 6 + 3] = base_index + 1;
        indices3D[face * 6 + 4] = base_index + 2;
        indices3D[face * 6 + 5] = base_index + 3;
    }

	glGenVertexArrays(1, &g_renderer.vao);
	glBindVertexArray(g_renderer.vao);

    Grass* testGrassBlock = new Grass();

    //Keep in mind:
    //program
    //vertexBuffer
    //indexBuffer
    //texture

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


        Rect imageSource = {
            .botLeft  = { 0 ,  1440 },
            .topRight = { 16 , 1456 },
        };
        Rect screenLocation = {
            .botLeft  = { -1, -1 },
            .topRight = {  1,  1 },
        };
        g_renderer.drawcalls.push_back(CreateDrawCall(GetRectFromSprite(0), screenLocation,
                            &g_renderer.textures["MinecraftSpriteSheet"], Shader::Simple2D));

        //imageSource = {
        //    .botLeft  = { 17 , 1440 },
        //    .topRight = { 32 , 1456 },
        //};
        //g_renderer.drawcalls.push_back(CreateDrawCall(imageSource, {},
        //                    &g_renderer.textures["MinecraftSpriteSheet"], Shader::Simple3D));

        testGrassBlock->Render();
        RenderUpdate();

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
