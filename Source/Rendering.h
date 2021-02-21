#pragma once
#include "SDL/include/SDL.h"
#include "glew.h"
#include "Math.h"
#include "Misc.h"

#include <string>
#include <Windows.h>

enum class ts_MessageBox {
	Invalid,
	Error = SDL_MESSAGEBOX_ERROR,
	Warning = SDL_MESSAGEBOX_WARNING,
	Informative = SDL_MESSAGEBOX_INFORMATION,
	Count,
};

enum class Shader : uint32 {
    Invalid,
    Simple2D,
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
	FILETIME m_vertexLastWriteTime = {};
	FILETIME m_pixelLastWriteTime = {};

    ShaderProgram(const ShaderProgram& rhs) = delete;
    ShaderProgram& operator=(const ShaderProgram& rhs) = delete;

    bool CompileShader(GLuint handle, const char* text);

public:
    ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation);
    ~ShaderProgram();
    FILETIME GetShaderTimeAndText(uint8* buffer, const size_t bufferLength, const std::string& fileLoc);
    void CheckForUpdate();
    void UseShader();
    void UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value);
};


int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char* title, const char* message);
