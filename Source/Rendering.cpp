#include "Rendering.h"
#include "Chunk.h"
#include "WinInterop.h"
#include "Input.h"

#include "tracy-master/Tracy.hpp"
#include "STB/stb_image.h"
#include "imgui.h"
#include "SDL/include/SDL.h"
#include "SDL/include/SDL_syswm.h"

Renderer g_renderer;
Window g_window;
//Camera g_camera;
#if DIRECTIONALLIGHT == 1
//Light_Direction g_light;
#else
Light_Point g_light;
#endif

extern "C" {
#ifdef _MSC_VER
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#else
    __attribute__((dllexport)) uint32_t NvOptimusEnablement = 0x00000001;
    __attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
}

const SDL_MessageBoxColorScheme colorScheme = {
    /* .colors (.r, .g, .b) */
       /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
    {{ 200, 200, 200 },
    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
    {   0,   0,   0 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
    { 100, 100, 100 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
    { 220, 220, 220 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
    { 240, 240, 240 }}
};

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char*  title, const char* message)
{
    SDL_MessageBoxData messageBoxData = {
        .flags = uint32(type),
        .window = NULL,
        .title = title, //an UTF-8 title
        .message = message, //an UTF-8 message text
        .numbuttons = numOfButtons, //the number of buttons
        .buttons = buttons, //an array of SDL_MessageBoxButtonData with length of numbuttons
        .colorScheme = &colorScheme
    };

    int32 buttonID = 0;

    if (SDL_ShowMessageBox(&messageBoxData, &buttonID))
    {
        FAIL;
    }
    if (buttonID == -1)
    {
        FAIL;
    }
    return buttonID;
}

ShaderProgram::~ShaderProgram()
{
    //TODO: Delete shaders as well
    glDeleteProgram(m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Program Deleted\n");
#endif
}
bool ShaderProgram::CompileShader(GLuint handle, std::string text, const std::string& fileName)
{
    const char* strings[] = { text.c_str() };
    glShaderSource(handle, 1, strings, NULL);
    glCompileShader(handle);

    GLint status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint log_length;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        std::string infoString;
        infoString.resize(log_length, 0);
        //GLchar info[4096] = {};
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)infoString.c_str());
        //std::string errorTitle = ToString("%s compilation error: %s\n", fileName.c_str(), infoString.c_str());
        std::string errorTitle = fileName + " Compilation Error: ";
        DebugPrint((errorTitle + infoString + "\n").c_str());

        SDL_MessageBoxButtonData buttons[] = {
            //{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
        };

        int32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), ts_MessageBox::Error, errorTitle.c_str(), infoString.c_str());
        if (buttons[buttonID].buttonid == 2)//NOTE: Stop button
        {
            DebugPrint("stop hit");
            g_running = false;
            return false;
        }
        else
        {
            if (buttons[buttonID].buttonid == 0)//NOTE: Continue button
            {
                return false;
            }
            else if (buttons[buttonID].buttonid == 1)//NOTE: Retry button
            {
                CheckForUpdate();
            }
        }
    }
#ifdef _DEBUGPRINT
    DebugPrint("Shader Vertex/Fragment Created\n");
#endif
    return true;
}

ShaderProgram::ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation)
{
    m_vertexFile = vertexFileLocation;
    m_pixelFile = pixelFileLocation;

    CheckForUpdate();
}
void ShaderProgram::UseShader()
{
    glUseProgram(m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Used\n");
#endif
}
void ShaderProgram::CheckForUpdate()
{
    std::string vertexText;
    uint64 vertexFileTime;
    std::string pixelText;
    uint64 pixelFileTime;
    {

        File vertexFile(m_vertexFile, File::Mode::Read, false);
        vertexFile.GetTime();
        if (!vertexFile.m_timeIsValid)
        {
            assert(false);
            return;
        }
        vertexFile.GetText();
        vertexText = vertexFile.m_dataString;
        vertexFileTime = vertexFile.m_time;

        File pixelFile(m_pixelFile, File::Mode::Read, false);
        pixelFile.GetTime();
        if (!pixelFile.m_timeIsValid)
        {
            assert(false);
            return;
        }
        pixelFile.GetText();
        pixelText = pixelFile.m_dataString;
        pixelFileTime = pixelFile.m_time;
    }

    if (m_vertexLastWriteTime < vertexFileTime ||
        m_pixelLastWriteTime  < pixelFileTime)
    {
        //Compile shaders and link to program
        GLuint vhandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint phandle = glCreateShader(GL_FRAGMENT_SHADER);

        if (!CompileShader(vhandle, vertexText, m_vertexFile) ||
            !CompileShader(phandle, pixelText, m_pixelFile))
            return;


        GLuint handle = glCreateProgram();

        glAttachShader(handle, vhandle);
        glAttachShader(handle, phandle);
        glLinkProgram(handle);

        GLint status;
        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint log_length;
            glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
            GLchar info[4096] = {};
            assert(log_length > 0);
            glGetProgramInfoLog(handle, log_length, NULL, info);
            DebugPrint("Shader linking error: %s\n", info);

            SDL_MessageBoxButtonData buttons[] = {
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
            };
            int32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), ts_MessageBox::Error, "Shader Compilation Error", reinterpret_cast<char*>(info));

            if (buttonID = 0)
            {
                CheckForUpdate();
            }
            else if (buttonID = 1)
            {
                g_running = false;
                return;
            }
            else
            {
                FAIL;
            }

        }
        else
        {
            m_handle = handle;
#ifdef _DEBUGPRINT
            DebugPrint("Shader Created\n");
#endif
            m_vertexLastWriteTime = vertexFileTime;
            m_pixelLastWriteTime = pixelFileTime;
            glDeleteShader(vhandle);
            glDeleteShader(phandle);
        }
    }
}

//TODO: Put somewhere appropriate
#ifdef _DEBUGPRINT
#define DEBUGLOG(...) DebugPrint(__VA_ARGS__)
#else
#define DEBUGLOG(...) ((void)0)
#endif

void ShaderProgram::UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniformMatrix4fv(loc, count, transpose, value);
    DEBUGLOG("Shader Uniform Updated %s\n", name);
}

void ShaderProgram::UpdateUniformVec4(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform4fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformVec3(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform3fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformVec2(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform2fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformFloat(const char* name, GLfloat value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1f(loc, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformInt2(const char* name, Vec2Int values)
{
    UpdateUniformInt2(name, GLint(values.x), GLint(values.y));
}

void ShaderProgram::UpdateUniformInt2(const char* name, GLint value1, GLint value2)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    //glUniform1f(loc, value);
    glUniform2i(loc, value1, value2);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformUint8(const char* name, GLuint value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1ui(loc, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformUintStream(const char* name, GLsizei count, GLuint* values)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1uiv(loc, count, values);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void GpuBuffer::UploadData(void* data, size_t size)
{
    Bind();
    size_t required_size = size;
    if (m_allocated_size < required_size)
    {
        glBufferData(m_target, required_size, nullptr, GL_STATIC_DRAW);
        m_allocated_size = required_size;
    }
    glBufferSubData(m_target, 0, required_size, data);
}

GpuBuffer::~GpuBuffer()
{
    glDeleteBuffers(1, &m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("GPU Buffer deleted %i, %i\n", m_target, m_handle);
#endif
}

void GpuBuffer::Bind()
{
    glBindBuffer(m_target, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("GPU Buffer Bound %i, %i\n", m_target, m_handle);
#endif
}

GLuint GpuBuffer::GetGLHandle()
{
    return m_handle;
}

void IndexBuffer::Upload(uint32* indices, size_t count)
{
    UploadData(indices, sizeof(indices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Index Buffer Upload,size %i\n", count);
#endif
}

void VertexBuffer::Upload(Vertex* vertices, size_t count)
{
    UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}
void VertexBuffer::Upload(Vertex_UI* vertices, size_t count)
{
    UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}
void VertexBuffer::Upload(Vertex_Chunk* vertices, size_t count)
{
    UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}

void DepthWrite(bool status)
{
    glDepthMask(status);
}
void DepthRead(bool status)
{
    status ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

double s_lastShaderUpdateTime = 0;
double s_incrimentalTime = 0;
void RenderUpdate(Vec2Int windowSize, float deltaTime)
{
    ZoneScopedN("Render Update");
    CheckFrameBufferStatus();
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_renderer.numTrianglesDrawn = 0;
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


    CheckFrameBufferStatus();
    g_framebuffers->Update(windowSize, g_renderer.maxMSAASamples, g_renderer.depthPeelingPasses, g_renderer.msaaEnabled);
    //UpdateFrameBuffers(windowSize, g_renderer.msaaEnabled ? g_renderer.maxMSAASamples : 1);
    g_framebuffers->m_opaque.Bind();
    //g_renderer.opaqueTarget->Bind();
    glViewport(0, 0, windowSize.x, windowSize.y);

    if (!g_renderer.usingDepthPeeling)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_framebuffers->m_transparent.Bind();

        Vec4 clearVec0 = Vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
        glClearBufferfv(GL_COLOR, 0, clearVec0.e);
        //Vec4 clearVec1 = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f }; //is this right or is only the first value 1.0f and the rest 0.0f?
        Vec4 clearVec1 = Vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
        glClearBufferfv(GL_COLOR, 1, clearVec1.e);

        g_framebuffers->m_opaque.Bind();
    }
}

Rect GetRectFromSprite(uint32 i)
{
    assert(i <= blocksPerRow * blocksPerRow);
    i = Clamp<uint32>(i, 0, blocksPerRow * blocksPerRow);
    uint32 x = i % blocksPerRow;
    uint32 y = i / blocksPerRow;

    Rect result = {};
    result.botLeft.x  = static_cast<float>(x * pixelsPerBlock);
    result.botLeft.y  = static_cast<float>(pixelsPerBlock * (blocksPerRow - y - 1));
    result.topRight.x = result.botLeft.x + pixelsPerBlock;
    result.topRight.y = result.botLeft.y + pixelsPerBlock;
    return result;
}

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

void FillIndexBuffer(IndexBuffer* ib)
{
    std::vector<uint32> arr;

    size_t amount = CHUNK_X * CHUNK_Y * CHUNK_Z * 6 * 6;
    arr.reserve(amount);
    int32 baseIndex = 0;
    for (int32 i = 0; i < amount; i += 6)
    {
        arr.push_back(baseIndex + 0);
        arr.push_back(baseIndex + 1);
        arr.push_back(baseIndex + 2);
        arr.push_back(baseIndex + 1);
        arr.push_back(baseIndex + 3);
        arr.push_back(baseIndex + 2);

        baseIndex += 4; //Amount of vertices
    }
    
    ib->Upload(arr.data(), amount);
}




VertexBuffer* UI_VertexBuffer = nullptr;

struct UI_DrawCall {
    int32 vertexIndex = 0;
    Color colorMod = { 1.0f, 1.0f, 1.0f, 1.0f };
    Texture::T texture;
};

std::vector<UI_DrawCall> UI_DrawCalls;
std::vector<Vertex_UI> UI_Vertices;

void UI_AddDrawCall(RectInt sourceRect, RectInt _destRect, Color colorMod, Texture::T textureType)
{
    Rect destRect;
    destRect.botLeft.x  = (_destRect.botLeft.x  / float(g_window.size.x)) * 2.0f - 1.0f;
    destRect.botLeft.y  = (_destRect.botLeft.y  / float(g_window.size.y)) * 2.0f - 1.0f;
    destRect.topRight.x = (_destRect.topRight.x / float(g_window.size.x)) * 2.0f - 1.0f;
    destRect.topRight.y = (_destRect.topRight.y / float(g_window.size.y)) * 2.0f - 1.0f;
    
    UI_AddDrawCall(sourceRect, destRect, colorMod, textureType);
}

void UI_AddDrawCall(RectInt _sourceRect, Rect destRect, Color colorMod, Texture::T textureType)
{
    Vec2Int textureSize = g_renderer.textures[textureType]->m_size;
    Rect sourceRect;

    if ((_sourceRect.botLeft.x == 0) && (_sourceRect.botLeft.y == 0) && (_sourceRect.topRight.x == 0) && (_sourceRect.topRight.y == 0))
    {
        _sourceRect.botLeft = {};
        _sourceRect.topRight = { textureSize.x, textureSize.y };
    }

    sourceRect.botLeft.x  = _sourceRect.botLeft.x  / float(textureSize.x);
    sourceRect.botLeft.y  = _sourceRect.botLeft.y  / float(textureSize.y);
    sourceRect.topRight.x = _sourceRect.topRight.x / float(textureSize.x);
    sourceRect.topRight.y = _sourceRect.topRight.y / float(textureSize.y);

    UI_DrawCall drawCall;
    drawCall.vertexIndex = int32(UI_Vertices.size());

    Vec2 p0 = { destRect.botLeft.x,  destRect.topRight.y };
    Vec2 p1 = { destRect.botLeft.x,  destRect.botLeft.y  };
    Vec2 p2 = { destRect.topRight.x, destRect.topRight.y };
    Vec2 p3 = { destRect.topRight.x, destRect.botLeft.y  };

    Vec2 uv0 = { sourceRect.botLeft.x,  sourceRect.topRight.y };
    Vec2 uv1 = { sourceRect.botLeft.x,  sourceRect.botLeft.y  };
    Vec2 uv2 = { sourceRect.topRight.x, sourceRect.topRight.y };
    Vec2 uv3 = { sourceRect.topRight.x, sourceRect.botLeft.y  };

    Vertex_UI v0;
    v0.p  = p0;
    v0.uv = uv0;
    Vertex_UI v1;
    v1.p  = p1;
    v1.uv = uv1;
    Vertex_UI v2;
    v2.p  = p2;
    v2.uv = uv2;
    Vertex_UI v3;
    v3.p  = p3;
    v3.uv = uv3;

    UI_Vertices.push_back(v0);
    UI_Vertices.push_back(v1);
    UI_Vertices.push_back(v2);
    UI_Vertices.push_back(v3);

    drawCall.colorMod = colorMod;
    drawCall.texture = textureType;
    UI_DrawCalls.push_back(drawCall);
}

void UI_Render()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    //glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunci(0, GL_ONE, GL_ONE);
    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    glBlendEquation(GL_FUNC_ADD);

    UI_VertexBuffer->Bind();
    UI_VertexBuffer->Upload(UI_Vertices.data(), UI_Vertices.size());

    ShaderProgram* sp = g_renderer.programs[+Shader::UI];
    sp->UseShader();

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_UI), (void*)offsetof(Vertex_UI, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_UI), (void*)offsetof(Vertex_UI, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    for (auto& drawCall : UI_DrawCalls)
    {
        Vec2 scale = { 1.0f, 1.0f };
        sp->UpdateUniformVec2("u_scale", 1, scale.e);
        sp->UpdateUniformVec4("u_color", 1, drawCall.colorMod.e);

        g_renderer.textures[drawCall.texture]->Bind();

        glDrawArrays(GL_TRIANGLE_STRIP, drawCall.vertexIndex, 4);
        g_renderer.numTrianglesDrawn += 2;
    }
    UI_Vertices.clear();
    UI_DrawCalls.clear();
}

//The UV's are not setup to accept real images
void DrawTriangles(const std::vector<Triangle>& triangles, Color color, const Mat4& view, const Mat4& perspective, bool depthWrite)
{
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32> indices;
    vertices.reserve(triangles.size());
   

    for (int32 i = 0; i < triangles.size(); i++)
    {
        Vertex a = {};
        Vec3 normal = triangles[i].Normal();

        //this is to bring the triangle outside of the object and avoid Z-Fighting
        Vec3 normalPlusOffset = normal * 0.03f;

        a.p = triangles[i].p0.p + normalPlusOffset;
        a.uv = { 0.0f, 0.0f };
        vertices.push_back(a);
        a.p = triangles[i].p1.p + normalPlusOffset;
        a.uv = { 1.0f, 0.0f };
        vertices.push_back(a);
        a.p = triangles[i].p2.p + normalPlusOffset;
        a.uv = { 1.0f, 1.0f };
        vertices.push_back(a);
        indices.push_back(i * 3 + 0);
        indices.push_back(i * 3 + 1);
        indices.push_back(i * 3 + 2);
    }
    vertexBuffer.Upload(vertices.data(), vertices.size());
    indexBuffer.Upload(indices.data(), indices.size());
    
    vertexBuffer.Bind();
    indexBuffer.Bind();
    //g_renderer.chunkIB->Bind();

    Mat4 transform;
    //gb_mat4_translate(&modelMatrix, { p.p.x, p.p.y, p.p.z });
    gb_mat4_identity(&transform);

    float scale1D = 1.0f;
    Vec3 scale = { scale1D, scale1D, scale1D };
    uint32 spriteIndices[+Face::Count] = { 0, 0, 0, 0, 0, 0 };

    //glDisable(GL_CULL_FACE);
    if (!depthWrite)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    g_renderer.textures[Texture::T::Plain]->Bind();
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, (GLuint)vertices.size(), GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += (uint32)vertices.size();
}



uint8 s_pixelTextureData[] = { 255, 255, 255, 255 };

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

    uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI /*SDL_WINDOW_MOUSE_CAPTURE | *//*SDL_WINDOW_MOUSE_FOCUS | *//*SDL_WINDOW_INPUT_GRABBED*/;

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
            DebugPrint("OpenGL could not set recommended version: %i.%i to %i.%i\n", majorVersionRequest, minorVersionRequest,
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
    //glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClearColor(0.263f, 0.706f, 0.965f, 0.0f);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLErrorCallback, NULL);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_FRAMEBUFFER_SRGB); 

    glGenVertexArrays(1, &g_renderer.vao);
    glBindVertexArray(g_renderer.vao);

#if 0
    g_renderer.textures[Texture::Minecraft] = new Texture("Assets/TestSpriteSheet.png");
    g_renderer.spriteTextArray = new TextureArray("Assets/TestSpriteSheet.png");
#else
    g_renderer.textures[Texture::MinecraftRGB] = new Texture("Assets/MinecraftSpriteSheet20120215Modified.png", GL_RGBA);
    g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215Modified.png", GL_SRGB8_ALPHA8);
    g_renderer.spriteTextArray = new TextureArray("Assets/MinecraftSpriteSheet20120215Modified.png");
    //g_renderer.textures[Texture::MinecraftRGB] = new Texture("Assets/MinecraftSpriteSheet20120215.png", GL_RGBA);
    //g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215.png", GL_SRGB8_ALPHA8);
    //g_renderer.spriteTextArray = new TextureArray("Assets/MinecraftSpriteSheet20120215.png");
#endif
    //g_renderer.skyBox = new TextureCube("Assets/skybox.dds");
    g_renderer.skyBoxNight = new TextureCube("Assets/skyboxNight.dds");
    g_renderer.skyBoxDay   = new TextureCube("Assets/sky.dds");//DayMinecraftSkybox2.dds");
    g_renderer.textures[Texture::T::Plain] = new Texture(s_pixelTextureData, { 1, 1 }, GL_RGBA);

    g_renderer.programs[+Shader::Chunk]             = new ShaderProgram("Source/Shaders/Chunk.vert",        "Source/Shaders/Chunk.frag");
    g_renderer.programs[+Shader::OpaqueChunk]       = new ShaderProgram("Source/Shaders/Chunk.vert",        "Source/Shaders/ChunkOpaque.frag");
    g_renderer.programs[+Shader::TransparentChunk]  = new ShaderProgram("Source/Shaders/Chunk.vert",        "Source/Shaders/ChunkTransparent.frag");
    g_renderer.programs[+Shader::Cube]              = new ShaderProgram("Source/Shaders/Cube.vert",         "Source/Shaders/Cube.frag");
    g_renderer.programs[+Shader::CubeDepthPeeling]  = new ShaderProgram("Source/Shaders/Cube.vert",         "Source/Shaders/CubeDepthPeeling.frag");
    g_renderer.programs[+Shader::TransparentCube]   = new ShaderProgram("Source/Shaders/Cube.vert",         "Source/Shaders/CubeTransparent.frag");
    g_renderer.programs[+Shader::BufferCopy]        = new ShaderProgram("Source/Shaders/BufferCopy.vert",   "Source/Shaders/BufferCopy.frag");
    g_renderer.programs[+Shader::BufferCopyAlpha]   = new ShaderProgram("Source/Shaders/BufferCopy.vert",   "Source/Shaders/BufferCopyAlpha.frag");
    g_renderer.programs[+Shader::Sun]               = new ShaderProgram("Source/Shaders/Sun.vert",          "Source/Shaders/Sun.frag");
    g_renderer.programs[+Shader::UI]                = new ShaderProgram("Source/Shaders/UI.vert",           "Source/Shaders/UI.frag");
    g_renderer.programs[+Shader::Composite]         = new ShaderProgram("Source/Shaders/BufferCopy.vert",   "Source/Shaders/Composite.frag");

#if DIRECTIONALLIGHT == 1
    g_renderer.sunLight.d = Normalize(Vec3({  0.0f, -1.0f,  0.0f }));
    g_renderer.sunLight.c = {  1.0f,  1.0f,  1.0f };
#else
    g_light.p = { 25.0f, 270.0f, 25.0f };
    g_light.c = {  1.0f,  1.0f,  1.0f };
#endif

    g_renderer.chunkIB = new IndexBuffer();
    FillIndexBuffer(g_renderer.chunkIB);

    g_renderer.msaaEnabled = true;
    glGetIntegerv(GL_MAX_SAMPLES, &g_renderer.maxMSAASamples);
    g_renderer.maxMSAASamples = Min(g_renderer.maxMSAASamples, 16);

    Vertex verticees[] =
    {
        {-1.0,  1.0, 0, 0.0f, 1.0f },
        {-1.0, -1.0, 0, 0.0f, 0.0f },
        { 1.0,  1.0, 0, 1.0f, 1.0f },
        { 1.0, -1.0, 0, 1.0f, 0.0f },
    };
    g_renderer.postVertexBuffer = new VertexBuffer();
    g_renderer.postVertexBuffer->Upload(verticees, arrsize(verticees));

    UI_VertexBuffer = new VertexBuffer();

    FrameBufferInit();
}

void ResolveTransparentChunkFrameBuffer()
{
    ResolveMSAAFramebuffer(&g_framebuffers->m_transparent, &g_framebuffers->m_transparentPost, GL_COLOR_BUFFER_BIT);
    ResolveMSAAFramebuffer(&g_framebuffers->m_transparent, &g_framebuffers->m_transparentPost, GL_COLOR_BUFFER_BIT, GL_COLOR_ATTACHMENT1);

    // set render states
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //bind opaque framebuffer
    g_framebuffers->m_post.Bind();
    //globalRenderer.transparentPostTarget.Bind();
    //globalRenderer.opaqueTarget.Bind();
    //glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);

    //use composite shader
    //compositeShader.use();
    ////////globalRenderer.programs[+Shader::Composite]->UseShader();
    g_renderer.programs[+Shader::Composite]->UseShader();

    // draw screen quad
    glActiveTexture(GL_TEXTURE0);
    g_framebuffers->m_transparentPost.m_color->Bind();
    glActiveTexture(GL_TEXTURE1);
    g_framebuffers->m_transparentPost.m_color2->Bind();

    g_renderer.postVertexBuffer->Bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderAlphaCopy(Texture* s, Texture* d)
{
    assert(s);
    assert(d);
    auto& alphaBuffer = g_framebuffers->m_bufferAlphaCopy;

    alphaBuffer.Bind();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, alphaBuffer.m_handle);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    s->Bind();
    glActiveTexture(GL_TEXTURE1);
    d->Bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glViewport(0, 0, g_window.size.x, g_window.size.y);

    g_renderer.programs[+Shader::BufferCopyAlpha]->UseShader();

    g_renderer.postVertexBuffer->Bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, d->m_target, d->m_handle, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    alphaBuffer.m_color->Bind();

    g_renderer.programs[+Shader::BufferCopy]->UseShader();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, alphaBuffer.m_color->m_target, alphaBuffer.m_color->m_handle, 0);
}
