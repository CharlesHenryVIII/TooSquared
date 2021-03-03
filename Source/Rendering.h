#pragma once
#include "SDL/include/SDL.h"
#include "glew.h"
#include "Math.h"
#include "Misc.h"

#include <string>

#define DIRECTIONALLIGHT 1

struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context = nullptr;
    bool hasAttention = true;
};

struct Camera {
    Vec3 p      = { 0.0f, 260.0f, 0.0f };
    Vec3 front  = { 0.0f, 0.0f, -1.0f };
    Vec3 up     = { 0.0f, 1.0f, 0.0f };
    Mat4 view;
    float yaw   = -90.0f;
    float pitch = 0.0f;
	int32 fogDistance = 40;
};

struct Light_Point {
    Vec3 p;
    Vec3 c = {1, 1, 1};
};

struct Light_Direction{
    Vec3 d;
    Vec3 c = {1, 1, 1};
};

struct Material {
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    float shininess;
};

enum class ts_MessageBox {
	Invalid,
	Error = SDL_MESSAGEBOX_ERROR,
	Warning = SDL_MESSAGEBOX_WARNING,
	Informative = SDL_MESSAGEBOX_INFORMATION,
	Count,
};

enum class Shader : uint32 {
    Invalid,
    Simple3D,
    BufferCopy,
    Count,
};
ENUMOPS(Shader);

class Texture {
public:
    enum T : uint32 {
        Invalid,
        Minecraft,
        Test,
        Count,
    };
    ENUMOPS(T);

    Vec2Int size = {};
    int32 n = 0;//bytes per pixel
    uint8* data = {};
    GLuint gl_handle = {};

	Texture(const char* fileLocation);
	inline void Bind();
};

class TextureArray {
public:

    Vec2Int size = {};
    GLuint gl_handle = {};


	TextureArray(const char* fileLocation);
	inline void Bind();
};

class ShaderProgram
{
    GLuint m_handle = 0;
    std::string m_vertexFile;
    std::string m_pixelFile;
	uint64 m_vertexLastWriteTime = {};
	uint64 m_pixelLastWriteTime = {};

    ShaderProgram(const ShaderProgram& rhs) = delete;
    ShaderProgram& operator=(const ShaderProgram& rhs) = delete;

    bool CompileShader(GLuint handle, const char* name, std::string text);

public:
    ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation);
    ~ShaderProgram();
    void CheckForUpdate();
    void UseShader();
    void UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value);
    void UpdateUniformVec4(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformVec3(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformFloat(const char* name, GLfloat value);
    void UpdateUniformUint8(const char* name, GLuint value);
};

class GpuBuffer
{
    GLuint m_target;
    size_t m_allocated_size;
    GLuint m_handle;

    GpuBuffer(const GpuBuffer& rhs) = delete;
    GpuBuffer& operator=(const GpuBuffer& rhs) = delete;

protected:
    GpuBuffer(GLuint target)
        : m_target(target)
        , m_allocated_size(0)
	{
		glGenBuffers(1, &m_handle);
#ifdef _DEBUGPRINT
		DebugPrint("GPU Buffer Created %i\n", m_target);
#endif
	}
	void UploadData(void* data, size_t size);

public:
    virtual ~GpuBuffer();
    void Bind();
    GLuint GetGLHandle();
};

class IndexBuffer : public GpuBuffer
{
public:

	IndexBuffer()
		: GpuBuffer(GL_ELEMENT_ARRAY_BUFFER)
	{ }
    void Upload(uint32* indices, size_t count);
};

class VertexBuffer : public GpuBuffer
{
public:
	VertexBuffer()
		: GpuBuffer(GL_ARRAY_BUFFER)
	{ }
    void Upload(Vertex* vertices, size_t count);
    void Upload(Vertex_Chunk* vertices, size_t count);
};


struct FrameBuffer {
	GLuint handle;
	GLuint colorHandle;
	GLuint depthHandle;
	Vec2Int size;
	VertexBuffer vertexBuffer;
};

struct Renderer {
    SDL_GLContext GL_Context = {};
    ShaderProgram* programs[+Shader::Count] = {};
    Texture* textures[Texture::Count] = {};
    IndexBuffer* squareIndexBuffer = nullptr;
    GLuint vao;
    IndexBuffer* chunkIB;
    TextureArray* spriteTextArray;
    FrameBuffer* backBuffer = nullptr;
};

extern Renderer g_renderer;
extern Window g_window;
extern Camera g_camera;
#if DIRECTIONALLIGHT == 1
extern Light_Direction g_light;
#else
extern Light_Point g_light;
#endif

const uint32 pixelsPerBlock = 16;
const uint32 blocksPerRow = 16;

struct Block;

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);
Rect GetRectFromSprite(uint32 i);
void RenderUpdate(float deltaTime);
void InitializeVideo();
void UpdateFrameBuffer(Vec2Int size);
