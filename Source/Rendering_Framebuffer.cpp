#include "Rendering_Framebuffer.h"
FrameBufferManager* g_framebuffers = nullptr;

void FrameBufferInit()
{
    g_framebuffers = new FrameBufferManager();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

FrameBuffer::FrameBuffer()
{
    glGenFramebuffers(1, &m_handle);
    Bind();
}

void CheckFrameBufferStatus()
{
    GLint err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (err != GL_FRAMEBUFFER_COMPLETE)
    {
        DebugPrint("Error: Frame buffer error: %d \n", err);
        assert(false);
    }
}

void FrameBufferManager::Update(const Vec2Int& size, const uint32 samples, const int32 depthPeelingPasses)
{
    if (size == m_size && samples == m_multisampleCount)
    {
        return;
    }
    m_size = size;
    m_multisampleCount = samples;

    {//Opaque:
        //FrameBuffer_Basic* opaque = g_FBM.m_opaque;
        m_opaque.m_multisample = true;
        m_opaque.m_size = m_size;
        m_opaque.CreateTexture(&m_opaque.m_color, GL_COLOR_ATTACHMENT0, GL_RGBA,            GL_RGBA);
        m_opaque.CreateTexture(&m_opaque.m_depth, GL_DEPTH_ATTACHMENT,  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
    }
    {//transparent
        m_transparent.m_multisample = true;
        m_transparent.m_size = m_size;
        m_transparent.CreateTexture(&m_transparent.m_color,  GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
        m_transparent.CreateTexture(&m_transparent.m_reveal, GL_COLOR_ATTACHMENT1, GL_R8,      GL_RED,  GL_FLOAT);
    }
    {//Transparent Post
        m_transparentPost.m_multisample = false;
        m_transparentPost.m_size = m_size;
        m_transparentPost.CreateTexture(&m_transparentPost.m_color,  GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
        m_transparentPost.CreateTexture(&m_transparentPost.m_color2, GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
        m_transparentPost.CreateTexture(&m_transparentPost.m_depth,  GL_COLOR_ATTACHMENT1, GL_R8,      GL_RED,  GL_FLOAT);
    }
    {//Post
        m_post.m_multisample = false;
        m_post.m_size = m_size;
        m_post.CreateTexture(&m_post.m_color, GL_COLOR_ATTACHMENT0, GL_RGBA, GL_RGBA);
        m_post.CreateTexture(&m_post.m_depth, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
    }
    {//DepthPeeling
        m_depthPeeling.m_multisample = true;
        m_depthPeeling.m_size = m_size;
        m_depthPeeling.CreateTexture(&m_depthPeeling.m_color, GL_COLOR_ATTACHMENT0, GL_RGBA, GL_RGBA);
        m_depthPeeling.CreateTexture(&m_depthPeeling.m_depth, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
    }
    {//Resolve Depth Peeling
        m_resolveDepthPeeling.m_multisample = false;
        m_resolveDepthPeeling.m_size = m_size;
        m_resolveDepthPeeling.CreateTexture(&m_resolveDepthPeeling.m_opaqueColor,  0,                   GL_RGBA,            GL_RGBA);
        m_resolveDepthPeeling.CreateTexture(&m_resolveDepthPeeling.m_peelingDepth, 0,                   GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
        m_resolveDepthPeeling.CreateTexture(&m_resolveDepthPeeling.m_opaqueDepth,  GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
        m_resolveDepthPeeling.m_peelingColors.clear();
        for (int32 i = 0; i < depthPeelingPasses; i++)
        {
            m_resolveDepthPeeling.m_peelingColors.push_back(nullptr);
            m_resolveDepthPeeling.CreateTexture(&m_resolveDepthPeeling.m_peelingColors[i], 0, GL_RGBA, GL_RGBA);
        }
    }

    CheckFrameBufferStatus();
}

void FrameBuffer::CreateTexture(Texture** textureMember, GLenum attachment, GLint internalFormat, GLenum format, GLenum type)
{
    assert(textureMember != nullptr);
    if ((*textureMember) != nullptr)
        delete *textureMember;

    Texture::TextureParams tp = {
        .size = g_framebuffers->m_size,
        .minFilter = GL_LINEAR,
        .magFilter = GL_LINEAR,
        .wrapS = GL_REPEAT,
        .wrapT = GL_REPEAT,
        .internalFormat = internalFormat,
        .format = format,
        .type = type,
        .samples = m_multisample ? g_framebuffers->m_multisampleCount : 1,
    };
    //m_texInfo.push_back(tp);
    
    *textureMember = new Texture(tp);

    if (attachment)
    {
        assert((attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15) || (attachment == GL_DEPTH_ATTACHMENT) || (attachment == GL_STENCIL_ATTACHMENT));
        Bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, (*textureMember)->m_target, (*textureMember)->m_handle, 0);
    }
}

// Copying the multisampled framebuffer to a standard texture resolves the multiple samples per pixel. This must be done before using the
// framebuffer for any read operations.
void ResolveMSAAFramebuffer(const FrameBuffer* read, FrameBuffer* write, GLbitfield copyMask, GLenum TextureToCopy)
{
    assert(read && write);
    if (read && write)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, read->m_handle);
        if (TextureToCopy)
            glReadBuffer(TextureToCopy);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, write->m_handle);
        if (TextureToCopy)
            glDrawBuffer(TextureToCopy);
        glBlitFramebuffer(0, 0, read->m_size.x, read->m_size.y, 0, 0, write->m_size.x, write->m_size.y, copyMask, GL_NEAREST);
    }
}
