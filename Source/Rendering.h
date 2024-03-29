#pragma once
#include "SDL/include/SDL.h"
#include "glew.h"
#include "Math.h"
#include "Misc.h"
#include "Rendering_Framebuffer.h"
#include "Rendering_Texture.h"
#include "Vox.h"

#include <string>

#define DIRECTIONALLIGHT 1

struct Renderer;
extern Renderer g_renderer;
struct Window;
extern Window g_window;
#if DIRECTIONALLIGHT == 1
//struct Light_Direction;
//extern Light_Direction g_light;
#else
struct Light_Point;
extern Light_Point g_light;
#endif
extern Vec3 g_ambientLight;


struct Window {
    Vec2Int size = {};
    Vec2Int pos = {};
    SDL_Window* SDL_Context = nullptr;
    bool hasAttention = true;
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
    Chunk,
    Composite,
    Cube,
    Block,
    BlockComplex,
    BufferCopy,
    BufferCopyAlpha,
    Sun,
    UI,
    Shader,
    Voxel,
    Count,
};
ENUMOPS(Shader);

enum class Mesh : uint32 {
    Invalid,
    Belt_Normal,
    Belt_Turn_CCW,
    Belt_Turn_CW,
    Belt_Up1,
    Belt_Up2,
    Count,
};
ENUMOPS(Mesh);

class ShaderProgram
{
    GLuint m_handle = 0;
    std::string m_vertexFile;
    std::string m_pixelFile;
    uint64 m_vertexLastWriteTime = {};
    uint64 m_pixelLastWriteTime = {};

    ShaderProgram(const ShaderProgram& rhs) = delete;
    ShaderProgram& operator=(const ShaderProgram& rhs) = delete;
    bool CompileShader(GLuint handle, std::string text, const std::string& fileName);

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
    void UpdateUniformFloatStream(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformInt2(const char* name, Vec2Int values);
    void UpdateUniformInt2(const char* name, GLint value1, GLint value2);
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
    size_t m_count = 0;
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
    void Upload(Vertex_Block* vertices, size_t count);
    void Upload(Vertex_Cube* vertices, size_t count);
    void Upload(Vertex_Complex* vertices, size_t count);
    void Upload(Vertex_Voxel* vertices, size_t count);
    template <typename T>
    void Upload(T* vertices, size_t count)
    {
        UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
        DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
    }
};


struct Renderer {
    SDL_GLContext GL_Context = {};
    ShaderProgram* programs[+Shader::Count] = {};
    Texture* textures[Texture::Count] = {};
    IndexBuffer* squareIndexBuffer = nullptr;
    GLuint vao;
    IndexBuffer* chunkIB;
    TextureArray* spriteTextArray;
    VertexBuffer* postVertexBuffer;
    uint32 numTrianglesDrawn = 0;
    TextureCube* skyBoxNight;
    TextureCube* skyBoxDay;
    Light_Direction sunLight;
    Light_Direction moonLight;
    bool msaaEnabled = true;
    int32 maxMSAASamples = 1;
    float maxAnisotropic;
    float currentAnisotropic = 1.0f;
    int32 depthPeelingPasses = 3;
    int32 debug_DepthPeelingPassToDisplay = -1;
    int32 swapInterval = 1;
    std::vector<Voxels> voxelModels[+Mesh::Count] = {};
    std::vector<VertexBuffer*> meshVertexBuffers[+Mesh::Count] = {};
    std::vector<IndexBuffer*> meshIndexBuffers[+Mesh::Count] = {};
};

const uint32 pixelsPerBlock = 16;
const uint32 blocksPerRow = 16;

struct Block;

void ResolveTransparentChunkFrameBuffer();

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);
void DepthWrite(bool status);
void DepthRead(bool status);
Rect GetRectFromSprite(uint32 i);
void RenderUpdate(Vec2Int windowSize, float deltaTime);
void InitializeVideo();
void CheckFrameBufferStatus();
void DrawTriangles(const std::vector<Triangle>& triangles, Color color, const Mat4& view, const Mat4& perspective, bool depthWrite, const int32 passCount);
void RenderAlphaCopy(Texture* source, Texture* destination);


//Rendering_Skybox.cpp
struct Camera;
void RenderSkybox(const Camera* playerCamera);
void UpdateHeavens(Light_Direction& sun, Light_Direction& moon, const float inGameTime);

//Rendering blocks
void RenderVoxMesh(WorldPos p, const float scale, const Camera* camera, const Mesh mesh, const int32 meshAnimationState, float rotationInRads, const int32 passCount, const Color& color = White);
void RenderVoxMesh(WorldPos p, const Vec3 scale,  const Camera* camera, const Mesh mesh, const int32 meshAnimationState, float rotationInRads, const int32 passCount, const Color& color = White);
