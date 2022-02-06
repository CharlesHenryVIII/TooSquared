#include "Rendering_Texture.h"

#include "STB/stb_image.h"

Texture::Texture(TextureParams tp)
{
    assert(tp.samples);
    if (tp.samples > 1)
    {
        m_target = GL_TEXTURE_2D_MULTISAMPLE;
    }

    glGenTextures(1, &m_handle);
    Bind();

    if (m_target != GL_TEXTURE_2D_MULTISAMPLE)
    {
        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, tp.minFilter);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, tp.magFilter);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_S, tp.wrapS);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_T, tp.wrapT);
    }

    if (tp.samples == 1)
        glTexImage2D(GL_TEXTURE_2D, 0, tp.internalFormat, tp.size.x, tp.size.y, 0, tp.format, tp.type, tp.data);
    else
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, tp.samples, tp.internalFormat, tp.size.x, tp.size.y, GL_TRUE); // NOTE: Could change to GL_FALSE to use custom sample locations

    m_size = tp.size;
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

Texture::Texture(const char* fileLocation, GLint colorFormat)
{
    m_data = stbi_load(fileLocation, &m_size.x, &m_size.y, &m_bytesPerPixel, STBI_rgb_alpha);

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(m_target, 0, colorFormat, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
    //glTexImage2D(m_target, 0, GL_RGBA, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

Texture::Texture(uint8* data, Vec2Int size, GLint colorFormat)//, int32 m_bytesPerPixel)
{
    m_data = data;
    m_size = size;
    m_bytesPerPixel = 4;

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(m_target, 0, colorFormat, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_handle);
    stbi_image_free(m_data);
}

void Texture::Bind()
{
    glBindTexture(m_target, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Bound\n");
#endif
}


TextureArray::TextureArray(const char* fileLocation)
{
    uint8* data = stbi_load(fileLocation, &m_size.x, &m_size.y, NULL, STBI_rgb_alpha);
    Defer{
        stbi_image_free(data);
    };
    assert(data);

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    uint32 mipMapLevels = 5;
    m_spritesPerSide = { 16, 16 };
    uint32 height = m_spritesPerSide.y;
    uint32 width = m_spritesPerSide.x;
    uint32 depth = 256;

    //glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_RGBA8, width, height, depth);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_SRGB8_ALPHA8, width, height, depth);

    //TODO: Fix
    uint32 colors[16 * 16] = {};
    uint32 arrayIndex = 0;
    for (uint32 y = height; y--;)
    {
        for (uint32 x = 0; x < width; x++)
        {
            for (uint32 xp= 0; xp < 16; xp++)  //Total
            {
                for (uint32 yp = 0; yp < 16; yp++)  //Total
                {
                    uint32 sourceIndex = (y * 16 + yp) * 256 + x * 16 + xp;
                    uint32 destIndex = yp * 16 + xp;
                    colors[destIndex] = reinterpret_cast<uint32*>(data)[sourceIndex];
                }
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, arrayIndex, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, colors);
            arrayIndex++;
        }
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

void TextureArray::Bind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Bound\n");
#endif
}


struct DDS_PIXELFORMAT
{
    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwFourCC;
    uint32 dwRGBBitCount;
    uint32 dwRBitMask;
    uint32 dwGBitMask;
    uint32 dwBBitMask;
    uint32 dwABitMask;
};
static_assert(sizeof(DDS_PIXELFORMAT) == 32, "Incorrect structure size!");

typedef struct
{
    uint32           dwSize;
    uint32           dwFlags;
    uint32           dwHeight;
    uint32           dwWidth;
    uint32           dwPitchOrLinearSize;
    uint32           dwDepth;
    uint32           dwMipMapCount;
    uint32           dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32           dwCaps;
    uint32           dwCaps2;
    uint32           dwCaps3;
    uint32           dwCaps4;
    uint32           dwReserved2;
} DDS_HEADER;
static_assert(sizeof(DDS_HEADER) == 124, "Incorrect structure size!");

TextureCube::TextureCube(const char* fileLocation)
{
    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    FILE* file;
    if (fopen_s(&file, fileLocation, "rb") != 0)
    {
        assert(false);
        return;
    }

    Defer{ fclose(file); };

    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* buffer = new uint8_t[size];
    fread(buffer, size, 1, file);
    DDS_HEADER* header = (DDS_HEADER*)((uint32*)buffer + 1);
    uint8_t* data = (uint8_t*)(header + 1);

    int32 levels = header->dwMipMapCount;

#if 0
    void glTexStorage3D(GL_PROXY_TEXTURE_CUBE_MAP_ARRAY,
                        levels,
                        GLenum internalformat,
                        GLsizei width,
                        GLsizei height,
                        GLsizei depth);
#endif

    GLenum targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < arrsize(targets); ++i)
    {
        GLsizei width = header->dwWidth;
        GLsizei height = header->dwHeight;

        auto Align = [](GLsizei i) { return i + 3 & ~(3); };

        for (int level = 0; level < levels; ++level)
        {
            GLsizei bw = Align(width);
            GLsizei bh = Align(height);
            GLsizei byte_size = bw * bh;

            glCompressedTexImage2D(targets[i],
                                    level,
                                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                    width,
                                    height,
                                    0,
                                    byte_size,
                                    data);

            data += byte_size;
            width = Max(width >> 1, 1);
            height = Max(height >> 1, 1);
        }
    }
}

void TextureCube::Bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
}

