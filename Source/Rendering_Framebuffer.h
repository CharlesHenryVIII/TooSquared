#pragma once
#include "glew.h"
#include "Math.h"
#include "Misc.h"
#include "Rendering_Texture.h"

struct FrameBuffer {
private:
    FrameBuffer(const FrameBuffer& rhs) = delete;
    FrameBuffer& operator=(const FrameBuffer& rhs) = delete;

public:
    GLuint m_handle = 0;
    Vec2Int m_size = {};
    bool m_multisample;

    FrameBuffer();
    void Bind() const;
    void CreateTexture(Texture** textureMember, GLenum attachment, GLint internalFormat, GLenum format, GLenum type = GL_UNSIGNED_BYTE);
};

struct FrameBuffer_Basic : FrameBuffer {
    Texture* m_color = nullptr;
    Texture* m_depth = nullptr;
};

struct FrameBuffer_Transparent : FrameBuffer {
    Texture* m_color = nullptr;
    Texture* m_reveal = nullptr;
};
struct FrameBuffer_TransparentPost : FrameBuffer {
    Texture* m_color = nullptr;
    Texture* m_color2 = nullptr;
    Texture* m_depth = nullptr;
};

struct FrameBuffer_ResolveDepthPeeling : FrameBuffer {
    Texture* m_opaqueColor = nullptr;
    Texture* m_peelingDepth = nullptr;
    Texture* m_opaqueDepth = nullptr;
    std::vector<Texture*> m_peelingColors;
};

class FrameBufferManager {
public:
    uint32 m_multisampleCount = 0;
    Vec2Int m_size = {};

    FrameBuffer_Basic               m_opaque;
    FrameBuffer_Transparent         m_transparent;
    FrameBuffer_TransparentPost     m_transparentPost;
    FrameBuffer_Basic               m_post;
    FrameBuffer_Basic               m_depthPeeling;
    FrameBuffer_ResolveDepthPeeling m_resolveDepthPeeling;

    void Update(const Vec2Int& size, const uint32 samples, const int32 depthPeelingPasses);
};
extern FrameBufferManager* g_framebuffers;

void CheckFrameBufferStatus();
void FrameBufferInit();
void ResolveMSAAFramebuffer(const FrameBuffer* read, FrameBuffer* write, GLbitfield copyMask, GLenum TextureToCopy = 0);
