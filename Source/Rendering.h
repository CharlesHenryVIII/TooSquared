#pragma once
#include "SDL/include/SDL.h"
#include "glew.h"
#include "Math.h"
#include "Misc.h"

#include <string>

#define DIRECTIONALLIGHT 1

struct Renderer;
extern Renderer g_renderer;
struct Window;
extern Window g_window;
//struct Camera;
//extern Camera g_camera;
#if DIRECTIONALLIGHT == 1
//struct Light_Direction;
//extern Light_Direction g_light;
#else
struct Light_Point;
extern Light_Point g_light;
#endif


struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context = nullptr;
    bool hasAttention = true;
};

//struct Camera {
//    Transform transform = {
//    .m_p = { 0.0f, 260.0f, 0.0f },
//    .m_pDelta = { 0.0f, 260.0f, 0.0f },
//    .m_vel = {},
//    .m_acceleration = {},
//    .m_terminalVel = 200.0f,
//    };
//    //WorldPos p  = { 0.0f, 260.0f, 0.0f };
//    Vec3 front  = { 0.0f, 0.0f, -1.0f };
//    Vec3 up     = { 0.0f, 1.0f, 0.0f };
//    Mat4 view;
//    float yaw   = -90.0f;
//    float pitch = 0.0f;
//    int32 fogDistance = 40;
//    int32 drawDistance = 10;
//};

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
    Chunk,
    Cube,
    BufferCopy,
    Sun,
    UI,
    Count,
};
ENUMOPS(Shader);

class Texture {
public:
    enum T : uint32 {
        Invalid,
        Minecraft,
        Test,
        Plain,
        Count,
    };
    ENUMOPS(T);

    struct TextureParams {
        Vec2Int size = g_window.size;
        uint32 minFilter = GL_LINEAR;
        uint32 magFilter = GL_LINEAR;
        uint32 wrapS = GL_REPEAT;
        uint32 wrapT = GL_REPEAT;
        GLint  internalFormat = GL_RGBA;
        GLenum format = GL_RGBA;
        GLenum type = GL_UNSIGNED_BYTE;
        uint32 samples = 1;

        void* data = nullptr;
    };


    Vec2Int m_size = {};
    int32 m_bytesPerPixel = 0;//bytes per pixel
    uint8* m_data = {};
    GLuint m_handle = {};
    GLenum m_target = GL_TEXTURE_2D;


    Texture(Texture::TextureParams tp);
    Texture(const char* fileLocation);
    Texture(uint8* data, Vec2Int size);//, int32 m_bytesPerPixel = 2);
    ~Texture();
    void Bind();
};

class TextureArray {
public:

    Vec2Int m_size = {};
    GLuint m_handle = {};
    Vec2Int m_spritesPerSide;


    TextureArray(const char* fileLocation);
    void Bind();
};

class TextureCube {
public:
    Vec2Int m_size = {};
    GLuint m_handle = {};


    TextureCube(const char* fileLocation);
    void Bind();
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
    void UpdateUniformVec2(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformFloat(const char* name, GLfloat value);
    void UpdateUniformUint8(const char* name, GLuint value);
    void UpdateUniformUintStream(const char* name, GLsizei count, GLuint* values);
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
    void Upload(Vertex_UI* vertices, size_t count);
    void Upload(Vertex_Chunk* vertices, size_t count);
};


class FrameBuffer {
    FrameBuffer(const FrameBuffer& rhs) = delete;
    FrameBuffer& operator=(const FrameBuffer& rhs) = delete;

public:
    GLuint m_handle = 0;
    Texture* m_color = nullptr;
    Texture* m_depth = nullptr;
    Vec2Int m_size = {};
    uint32  m_samples = 1;

    FrameBuffer();
    void Bind();
    void CreateTextures(Vec2Int size, uint32 samples);
};

struct Renderer {
    SDL_GLContext GL_Context = {};
    ShaderProgram* programs[+Shader::Count] = {};
    Texture* textures[Texture::Count] = {};
    IndexBuffer* squareIndexBuffer = nullptr;
    GLuint vao;
    IndexBuffer* chunkIB;
    TextureArray* spriteTextArray;
    FrameBuffer* sceneTarget = nullptr;
    FrameBuffer* postTarget  = nullptr;
    VertexBuffer* postVertexBuffer;
    VertexBuffer* cubeVertexBuffer;
    uint32 numTrianglesDrawn = 0;
    TextureCube* skyBoxNight;
    TextureCube* skyBoxDay;
    Light_Direction sunLight;
    Light_Direction moonLight;
    bool msaaEnabled = false;
    int32 maxMSAASamples = 1;
};

const uint32 pixelsPerBlock = 16;
const uint32 blocksPerRow = 16;

struct Block;

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);
Rect GetRectFromSprite(uint32 i);
void RenderUpdate(Vec2Int windowSize, float deltaTime);
void InitializeVideo();
void UpdateFrameBuffers(Vec2Int size, uint32 samples);
void ResolveMSAAFramebuffer();
void UI_AddDrawCall(RectInt sourceRect, RectInt _destRect, Color colorMod, Texture::T textureType);
void UI_AddDrawCall(RectInt _sourceRect, Rect destRect, Color colorMod, Texture::T textureType);
void UI_Render();
