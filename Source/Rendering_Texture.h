#pragma once
#include "glew.h"
#include "Math.h"
#include "Misc.h"

class Texture {
public:
    enum T : uint32 {
        Invalid,
        Minecraft,
        MinecraftRGB,
        Test,
        Plain,
        Count,
    };
    ENUMOPS(T);

    struct TextureParams {
        Vec2Int size = { 1920, 1080 }; //Always overwrite
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
    Texture(const char* fileLocation, GLint colorFormat);
    Texture(uint8* data, Vec2Int size, GLint colorFormat);//, int32 m_bytesPerPixel = 2);
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
