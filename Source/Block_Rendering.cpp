#include "Block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"

///
/// Add Cubes To Render
///
std::vector<Vertex_Cube> s_cubesToDraw_transparent;
std::vector<Vertex_Cube> s_cubesToDraw_opaque;

void AddCubeToRender(WorldPos p, Color color, float scale)
{
    AddCubeToRender(p, color, { scale, scale, scale });
}
void AddCubeToRender(WorldPos p, Color color, Vec3  scale)
{
    Vertex_Cube c {
        .p = p.p,
        .color = { color.r, color.g, color.b, color.a },
        .scale = scale,
    };

    auto* list = &s_cubesToDraw_opaque;
    if (color.a != 1.0f)
        list = &s_cubesToDraw_transparent;
    for (int32 f = 0; f < +Face::Count; f++)
        for (int32 v = 0; v < 4; v++)
        {
            list->push_back(c);
        }
}
void RenderCubesInternal(Camera* playerCamera, const int32 passCount, std::vector<Vertex_Cube>* cubesToDraw, bool clearCubesToDraw)
{
    if (cubesToDraw->size() == 0)
        return;
    g_renderer.chunkIB->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.textures[Texture::T::Plain]->Bind();
    VertexBuffer vBuffer = VertexBuffer();
    {
        ZoneScopedN("Upload Cubes");
        vBuffer.Upload(cubesToDraw->data(), cubesToDraw->size());
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        sp->UpdateUniformUint8("u_passCount", passCount);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, color));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, scale));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);
    {
        ZoneScopedN("Render Cubes");
        glDrawElements(GL_TRIANGLES, (GLsizei)((cubesToDraw->size() / 24) * 36), GL_UNSIGNED_INT, 0);
    }
    g_renderer.numTrianglesDrawn += 12 * (uint32)cubesToDraw->size();

    if (clearCubesToDraw)
        cubesToDraw->clear();
}
void RenderTransparentCubes(Camera* playerCamera, const int32 passCount, bool lastPass)
{
    ZoneScopedN("Upload and Render Transparent Cubes");
    RenderCubesInternal(playerCamera, passCount, &s_cubesToDraw_transparent, lastPass);
}

void RenderOpaqueCubes(Camera* playerCamera, const int32 passCount)
{
    ZoneScopedN("Upload and Render Opaque Cubes");
    RenderCubesInternal(playerCamera, 0, &s_cubesToDraw_opaque, true);
}

struct ComplexBlockDraw
{
    WorldPos    p;
    Vec3        scale;
    Color       color;
    float       rotation;
    BlockType   blockType;
};

std::vector<Vertex_Block> s_blocksToDraw_transparent;
std::vector<Vertex_Block> s_blocksToDraw_opaque;
std::vector<ComplexBlockDraw> s_blocksToDraw_complexTransparent;
std::vector<ComplexBlockDraw> s_blocksToDraw_complexOpaque;
void AddBlockToRender(const WorldPos& p, float scale, BlockType block, const Color& c, const Vec3& forward)
{
    AddBlockToRender(p, { scale, scale, scale }, block, c, forward);
}
void AddBlockToRender(const WorldPos& p, const Vec3& scale, BlockType block, const Color& c, const Vec3& forward)
{
    if (g_blocks[+block].m_flags & BLOCK_COMPLEX)
    {
        float rot = CoordinalPointToRad(ForwardVectorToCoordinalPoint(forward));
        WorldPos pF = p.p - (scale * 0.5f);
        if (c.a == 1.0f)
            s_blocksToDraw_complexOpaque.push_back(     { pF, scale, c, rot, block });
        else
            s_blocksToDraw_complexTransparent.push_back({ pF, scale, c, rot, block });

    }
    else
    {
        Vertex_Block b = {};
        b.p = p.p;
        b.scale = scale;
        b.color = c;
        auto* list = &s_blocksToDraw_opaque;
        if ((g_blocks[+block].m_flags & BLOCK_TRANSLUCENT) || (c.a != 1.0f))
            list = &s_blocksToDraw_transparent;
        for (int32 f = 0; f < +Face::Count; f++)
        {
            b.index = g_blocks[+block].m_spriteIndices[f];
            for (int32 v = 0; v < 4; v++)
                list->push_back(b);
        }
    }
}

void RenderBlocksInternal(Camera* playerCamera, const int32 passCount, std::vector<Vertex_Block>* blocksToDraw, bool clearBlocksToDraw)
{
    if (blocksToDraw->size() == 0)
        return;
    g_renderer.chunkIB->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Block];
    sp->UseShader();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.spriteTextArray->Bind();
    VertexBuffer vBuffer = VertexBuffer();
    {
        ZoneScopedN("Upload Blocks");
        vBuffer.Upload(blocksToDraw->data(), blocksToDraw->size());
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        sp->UpdateUniformUint8("u_passCount", passCount);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, scale));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, color));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, index));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);
    {
        ZoneScopedN("Render Blocks");
        glDrawElements(GL_TRIANGLES, (GLsizei)((blocksToDraw->size() / 24) * 36), GL_UNSIGNED_INT, 0);
    }
    g_renderer.numTrianglesDrawn += 12 * (uint32)blocksToDraw->size();

    if (clearBlocksToDraw)
        blocksToDraw->clear();
}
void RenderComplexBlocksInternal(const Camera* playerCamera, const int32 passCount, std::vector<ComplexBlockDraw>* list, bool lastPass)
{
    for (int32 i = 0; i < list->size(); i++)
    {
        auto& b = (*list)[i];
        Mesh mesh = Mesh::Invalid;
        int32 meshAnimationState = 0;
        switch (b.blockType)
        {
        case BlockType::Belt:
        {
            mesh = Mesh::Belt_Normal;
            break;
        }
        default: assert(false); break;
        }
        RenderVoxMesh(b.p, b.scale, playerCamera, mesh, meshAnimationState, b.rotation, passCount, b.color);
    }
    if (lastPass)
        list->clear();
}
void RenderTransparentBlocks(Camera* playerCamera, const int32 passCount, bool lastPass)
{
    ZoneScopedN("Upload and Render Transparent Blocks");
    RenderBlocksInternal(playerCamera, passCount, &s_blocksToDraw_transparent, lastPass);
    RenderComplexBlocksInternal(playerCamera, passCount, &s_blocksToDraw_complexTransparent, lastPass);
}
void RenderOpaqueBlocks(Camera* playerCamera, const int32 passCount)
{
    ZoneScopedN("Upload and Render Opaque Blocks");
    RenderBlocksInternal(playerCamera, passCount, &s_blocksToDraw_opaque, true);
    RenderComplexBlocksInternal(playerCamera, passCount, &s_blocksToDraw_complexOpaque, true);
}
