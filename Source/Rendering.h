#pragma once
#include "SDL/include/SDL.h"
#include "glew.h"
#include "Math.h"
#include "Misc.h"

#include <string>
#include <Windows.h>
//TODO: Create a union with a uint64 and FILETIME and
//get rid of the Windows.h dependency in the header

struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context = nullptr;
    bool hasAttention = true;
};

#ifdef CAMERA
struct Camera {
    Vec3 p      = { 0.0f, 3.0f, 3.0f };
    Vec3 front  = { 0.0f, 0.0f, -1.0f };
    Vec3 up     = { 0.0f, 1.0f, 0.0f };
    Mat4 view;
    float yaw   = -90.0f;
    float pitch = 0.0f;
};
#else
struct Camera {
    Vec3 p = { 2, 2, 2 };
    Vec3 r = {};
    Mat4 view;
};
#endif

struct Light {
    Vec3 p;
    Vec3 c;
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
};

struct Renderer {
    SDL_GLContext GL_Context = {};
    ShaderProgram* programs[+Shader::Count] = {};
    Texture* textures[Texture::Count] = {};
    IndexBuffer* squareIndexBuffer = nullptr;
    GLuint vao;
};

extern Renderer g_renderer;
extern Window g_window;
extern Camera g_camera;
extern Light g_light;

struct Block;

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);
void RenderUpdate(float deltaTime);
void RenderBlock(Block* block);
void InitializeVideo();
