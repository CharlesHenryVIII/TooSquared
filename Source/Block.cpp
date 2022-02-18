#include "Block.h"
#include "Entity.h"
#include "Math.h"

Block g_blocks[+BlockType::Count] = {};

static const Vec2 faceUV[4] = {
    Vec2{ 0, 1 },
    Vec2{ 0, 0 },
    Vec2{ 1, 1 },
    Vec2{ 1, 0 }
};

void SetMultipleBlockSprites(BlockType bt, uint32 v)
{
    for (uint32 i = 0; i < +Face::Count; i++)
    {
        g_blocks[+bt].m_spriteIndices[i] = v;
    }
}
void SetBlockSprites()
{
    g_blocks[+BlockType::Empty].m_translucent = true;
    g_blocks[+BlockType::Empty].m_collidable  = false;
    g_blocks[+BlockType::Empty].m_hasShading  = false;

    SetMultipleBlockSprites(BlockType::Dirt, 2);
    SetMultipleBlockSprites(BlockType::Grass, 3);
    g_blocks[+BlockType::Grass].m_spriteIndices[+Face::Top] = 0;
    g_blocks[+BlockType::Grass].m_spriteIndices[+Face::Bot] = 2;
    SetMultipleBlockSprites(BlockType::Stone, 1);
    SetMultipleBlockSprites(BlockType::IronBlock, 22);
    SetMultipleBlockSprites(BlockType::GoldBlock, 23);
    SetMultipleBlockSprites(BlockType::DiamondBlock, 24);
    SetMultipleBlockSprites(BlockType::Sand, 18);
    SetMultipleBlockSprites(BlockType::Snow, 66);
    SetMultipleBlockSprites(BlockType::Wood, 20);
    g_blocks[+BlockType::Wood].m_spriteIndices[+Face::Top] = 21;
    g_blocks[+BlockType::Wood].m_spriteIndices[+Face::Bot] = 21;
    SetMultipleBlockSprites(BlockType::Ice, 67);
    g_blocks[+BlockType::Ice].m_translucent = true;
    g_blocks[+BlockType::Leaves].m_sidesShouldBeRendered = false;
    SetMultipleBlockSprites(BlockType::Obsidian, 37);
#if 1
    SetMultipleBlockSprites(BlockType::Leaves, 52);
    g_blocks[+BlockType::Leaves].m_seeThrough = true;
    //g_blocks[+BlockType::Leaves].m_translucent = true; //This is because the mipmaps contain alpha transparency
    g_blocks[+BlockType::Leaves].m_translucent = false;
    g_blocks[+BlockType::Leaves].m_sidesShouldBeRendered = true;
#else
    SetMultipleBlockSprites(BlockType::Leaves, 53);
#endif
    SetMultipleBlockSprites(BlockType::MossyCobblestone, 36);
    SetMultipleBlockSprites(BlockType::TNT, 8);
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Top] = 9;
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Bot] = 10;
    SetMultipleBlockSprites(BlockType::Water, 255);
    g_blocks[+BlockType::Water].m_seeThrough  = true;
    g_blocks[+BlockType::Water].m_translucent = true;
    g_blocks[+BlockType::Water].m_collidable  = false;
    SetMultipleBlockSprites(BlockType::Bedrock, 17);

    SetMultipleBlockSprites(BlockType::HalfSlab, 5);
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Bot] = 6;
    g_blocks[+BlockType::HalfSlab].m_collisionHeight           = 0.5f;
    g_blocks[+BlockType::HalfSlab].m_seeThrough              = true;

    SetMultipleBlockSprites(BlockType::Slab, 5);
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Bot] = 6;

    SetMultipleBlockSprites(BlockType::Glass, 49);
    g_blocks[+BlockType::Glass].m_seeThrough            = true;
    g_blocks[+BlockType::Glass].m_translucent           = true;//This is because the mipmaps contain alpha transparency
    g_blocks[+BlockType::Glass].m_sidesShouldBeRendered = false;
    g_blocks[+BlockType::Glass].m_hasShading            = false;
}

void BlockInit()
{
    SetBlockSprites();
}

Rect GetUVsFromIndex(uint8 index)
{
    int32 spritesPerSide = 16;
    const Vec2Int& size = g_renderer.textures[Texture::Minecraft]->m_size;

    //auto blockIndex = blockSprites[+block].faceSprites[+Face::Top];
    //if (block == BlockType::Empty)
    //    blockIndex = 31;

    int32 x = index % spritesPerSide;
    int32 y = index / spritesPerSide;
    Vec2Int pixelsPerSprite = size / spritesPerSide;
    RectInt UVs = {
        .botLeft  = { x * pixelsPerSprite.x, (spritesPerSide - y) * pixelsPerSprite.y },
        .topRight = { UVs.botLeft.x + pixelsPerSprite.x, UVs.botLeft.y - pixelsPerSprite.y },
    };
    Rect result = {
        .botLeft  = { UVs.botLeft.x  / float(size.x), UVs.botLeft.y  / float(size.y) },
        .topRight = { UVs.topRight.x / float(size.x), UVs.topRight.y / float(size.y) },
    };
    return result;
}




///
/// Add Cubes To Render
///
std::vector<RenderCube> s_cubesToDraw_transparent;
std::vector<RenderCube> s_cubesToDraw_opaque;

void AddCubeToRender(WorldPos p, Color color, float scale)
{
    AddCubeToRender(p, color, { scale, scale, scale });
}
void AddCubeToRender(WorldPos p, Color color, Vec3  scale)
{
    RenderCube c {
        .p = p,
        .color = color,
        .scale = scale,
    };

    if (color.a == 1.0f)
        s_cubesToDraw_opaque.push_back(c);
    else
        s_cubesToDraw_transparent.push_back(c);
}
void RenderTransparentCubes(Camera* playerCamera, const int32 passCount, bool lastPass)
{
    for (int32 i = 0; i < s_cubesToDraw_transparent.size(); i++)
    {
        RenderCube& currentCube = s_cubesToDraw_transparent[i];
        VertexBuffer vb = VertexBuffer();
        Vertex vertices[sizeof(cubeVertices) / sizeof(cubeVertices[0].e[0])] = {};
        for (int32 f = 0; f < +Face::Count; f++)
            for (int32 i = 0; i < arrsize(VertexFace::e); i++)
            {
                int32 index = f * arrsize(VertexFace::e) + i;
                vertices[index].p = cubeVertices[f].e[i] - 0.5f;
                auto spriteIndex = g_blocks[+BlockType::Empty].m_spriteIndices[f];
                //TODO: Refactor this garbago:
                Rect UVSquare = GetUVsFromIndex(spriteIndex);
                vertices[index].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i].x);
                vertices[index].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i].y);
            }

        vb.Upload(vertices, arrsize(vertices));
        g_renderer.chunkIB->Bind();
        ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
        sp->UseShader();
        glActiveTexture(GL_TEXTURE0);
        g_renderer.textures[+Texture::T::Plain]->Bind();
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        Mat4 modelMatrix;
        gb_mat4_translate(&modelMatrix, currentCube.p.p);
        sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
        sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
        sp->UpdateUniformVec4("u_color", 1, currentCube.color.e);
        sp->UpdateUniformUint8("u_passCount", passCount);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glDisableVertexArrayAttrib(g_renderer.vao, 2);
        glDisableVertexArrayAttrib(g_renderer.vao, 3);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        g_renderer.numTrianglesDrawn += 36 / 3;
    }
    if (lastPass)
        s_cubesToDraw_transparent.clear();
}

void RenderOpaqueCubes(Camera* playerCamera, const int32 passCount)
{
    for (int32 i = 0; i < s_cubesToDraw_opaque.size(); i++)
    {
        RenderCube& currentCube = s_cubesToDraw_opaque[i];
        VertexBuffer vb = VertexBuffer();
        Vertex vertices[sizeof(cubeVertices) / sizeof(cubeVertices[0].e[0])] = {};
        for (int32 f = 0; f < +Face::Count; f++)
            for (int32 i = 0; i < arrsize(VertexFace::e); i++)
            {
                int32 index = f * arrsize(VertexFace::e) + i;
                vertices[index].p = cubeVertices[f].e[i] - 0.5f;
                auto spriteIndex = g_blocks[+BlockType::Empty].m_spriteIndices[f];
                //TODO: Refactor this garbago:
                Rect UVSquare = GetUVsFromIndex(spriteIndex);
                vertices[index].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i].x);
                vertices[index].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i].y);
            }

        vb.Upload(vertices, arrsize(vertices));
        g_renderer.chunkIB->Bind();
        ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
        sp->UseShader();
        glActiveTexture(GL_TEXTURE0);
        g_renderer.textures[+Texture::T::Plain]->Bind();
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        Mat4 modelMatrix;
        gb_mat4_translate(&modelMatrix, currentCube.p.p);
        sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
        sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
        sp->UpdateUniformVec4("u_color", 1, currentCube.color.e);
        sp->UpdateUniformUint8("u_passCount", passCount);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glDisableVertexArrayAttrib(g_renderer.vao, 2);
        glDisableVertexArrayAttrib(g_renderer.vao, 3);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        g_renderer.numTrianglesDrawn += 36 / 3;
    }
    s_cubesToDraw_opaque.clear();
}

#if DEBUG_BLOCKRENDER == 2
std::vector<RenderBlock> s_blocksToDraw_transparent;
std::vector<RenderBlock> s_blocksToDraw_opaque;
#else
std::vector<Vertex_Block> s_blocksToDraw_transparent;
std::vector<Vertex_Block> s_blocksToDraw_opaque;
#endif
void AddBlockToRender(WorldPos p, float scale, BlockType block)
{
    AddBlockToRender(p, { scale, scale, scale }, block);
}
void AddBlockToRender(WorldPos p, Vec3 scale, BlockType block)
{
#if DEBUG_BLOCKRENDER == 2
    RenderBlock b = {};
    b.p = p.p;
    b.scale = scale;
    b.block = block;
    auto* list = &s_blocksToDraw_opaque;
    if (g_blocks[+block].m_translucent)
        list = &s_blocksToDraw_transparent;
    list->push_back(b);
#else
    Vertex_Block b = {};
    b.p = p.p;
    b.scale = scale;
    auto* list = &s_blocksToDraw_opaque;
    if (g_blocks[+block].m_translucent)
        list = &s_blocksToDraw_transparent;
    for (int32 f = 0; f < +Face::Count; f++)
        for (int32 v = 0; v < 4; v++)
        {
            b.index = g_blocks[+block].m_spriteIndices[f];
            list->push_back(b);
        }
#endif
}

void RenderTransparentBlocks(Camera* playerCamera, const int32 passCount, bool lastPass)
{
    ZoneScopedN("Upload and Render Transparent Blocks");
    if (s_blocksToDraw_transparent.size() == 0)
        return;
#if DEBUG_BLOCKRENDER == 1
    VertexBuffer vBuffer = VertexBuffer();
    vBuffer.Upload(s_blocksToDraw_transparent.data(), s_blocksToDraw_transparent.size());
    g_renderer.chunkIB->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Block];
    sp->UseShader();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.spriteTextArray->Bind(); //g_renderer.textures[+Texture::T::Minecraft]->Bind();
    sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
    sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
    //Mat4 modelMatrix;
    //gb_mat4_translate(&modelMatrix, currentCube.p.p);
    //sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
    //sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
    //sp->UpdateUniformVec4("u_color", 1, White.e);
    sp->UpdateUniformUint8("u_passCount", passCount);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, scale));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, index));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);
    glDrawElements(GL_TRIANGLES, (GLsizei)s_blocksToDraw_transparent.size(), GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;

#elif DEBUG_BLOCKRENDER == 2
    for (int32 i = 0; i < s_blocksToDraw_transparent.size(); i++)
    {
        RenderBlock& currentCube = s_blocksToDraw_transparent[i];
        VertexBuffer vb = VertexBuffer();
        Vertex vertices[sizeof(cubeVertices) / sizeof(cubeVertices[0].e[0])] = {};
        for (int32 f = 0; f < +Face::Count; f++)
            for (int32 i = 0; i < arrsize(VertexFace::e); i++)
            {
                int32 index = f * arrsize(VertexFace::e) + i;
                vertices[index].p = cubeVertices[f].e[i] - 0.5f;
                auto spriteIndex = g_blocks[+currentCube.block].m_spriteIndices[f];
                //TODO: Refactor this garbago:
                Rect UVSquare = GetUVsFromIndex(spriteIndex);
                vertices[index].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i].x);
                vertices[index].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i].y);
            }

        vb.Upload(vertices, arrsize(vertices));
        g_renderer.chunkIB->Bind();
        ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
        sp->UseShader();
        glActiveTexture(GL_TEXTURE0);
        g_renderer.textures[+Texture::T::Minecraft]->Bind();
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        Mat4 modelMatrix;
        gb_mat4_translate(&modelMatrix, currentCube.p.p);
        sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
        sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
        sp->UpdateUniformVec4("u_color", 1, White.e);
        sp->UpdateUniformUint8("u_passCount", passCount);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glDisableVertexArrayAttrib(g_renderer.vao, 2);
        glDisableVertexArrayAttrib(g_renderer.vao, 3);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        g_renderer.numTrianglesDrawn += 36 / 3;
    }
#endif
    if (lastPass)
        s_blocksToDraw_transparent.clear();
}
//std::vector<Vec3>   positions;
//std::vector<Vec3>   scale;
//std::vector<uint32> uvIndicies;
std::vector<RenderCube> s_blocksToDraw_Old;
void RenderOpaqueBlocks(Camera* playerCamera, const int32 passCount)
{
    ZoneScopedN("Upload and Render Opaque Blocks");
#if 1
#if DEBUG_BLOCKRENDER == 1
    if (s_blocksToDraw_opaque.size() == 0)
        return;
    VertexBuffer vBuffer = VertexBuffer();
    vBuffer.Upload(s_blocksToDraw_opaque.data(), s_blocksToDraw_opaque.size());
    g_renderer.chunkIB->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Block];
    sp->UseShader();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.spriteTextArray->Bind(); //g_renderer.textures[+Texture::T::Minecraft]->Bind();
    sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
    sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
    //Mat4 modelMatrix;
    //gb_mat4_translate(&modelMatrix, currentCube.p.p);
    //sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
    //sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
    //sp->UpdateUniformVec4("u_color", 1, White.e);
    sp->UpdateUniformUint8("u_passCount", passCount);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, scale));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, index));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);
    glDrawElements(GL_TRIANGLES, (GLsizei)s_blocksToDraw_opaque.size(), GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
#elif DEBUG_BLOCKRENDER == 2
    for (int32 i = 0; i < s_blocksToDraw_opaque.size(); i++)
    {
        RenderBlock& currentCube = s_blocksToDraw_opaque[i];
        VertexBuffer vb = VertexBuffer();
        Vertex vertices[sizeof(cubeVertices) / sizeof(cubeVertices[0].e[0])] = {};
        for (int32 f = 0; f < +Face::Count; f++)
            for (int32 i = 0; i < arrsize(VertexFace::e); i++)
            {
                int32 index = f * arrsize(VertexFace::e) + i;
                vertices[index].p = cubeVertices[f].e[i] - 0.5f;
                auto spriteIndex = g_blocks[+currentCube.block].m_spriteIndices[f];
                //TODO: Refactor this garbago:
                Rect UVSquare = GetUVsFromIndex(spriteIndex);
                vertices[index].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i].x);
                vertices[index].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i].y);
            }

        vb.Upload(vertices, arrsize(vertices));
        g_renderer.chunkIB->Bind();
        ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
        sp->UseShader();
        glActiveTexture(GL_TEXTURE0);
        g_renderer.textures[+Texture::T::Minecraft]->Bind();
        sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
        Mat4 modelMatrix;
        gb_mat4_translate(&modelMatrix, currentCube.p.p);
        sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
        sp->UpdateUniformVec3("u_scale", 1, currentCube.scale.e);
        sp->UpdateUniformVec4("u_color", 1, White.e);
        sp->UpdateUniformUint8("u_passCount", passCount);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexArrayAttrib(g_renderer.vao, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(g_renderer.vao, 1);
        glDisableVertexArrayAttrib(g_renderer.vao, 2);
        glDisableVertexArrayAttrib(g_renderer.vao, 3);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        g_renderer.numTrianglesDrawn += 36 / 3;
    }
#endif

#else
    g_renderer.cubeVertexBuffer->Bind();
    g_renderer.chunkIB->Bind();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.textures[+Texture::T::Minecraft]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::CubeInstanced];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
    sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
    sp->UpdateUniformUint8("u_passCount", passCount);
#if 1
    for (const auto& rb : s_blocksToDraw_opaque)
    {
        positions.push_back(rb.p.p);
        scale.push_back(rb.scale);
        
        uint32 a = g_blocks[+rb.block].m_spriteIndices[0] & 0xF000;
        a |= (g_blocks[+rb.block].m_spriteIndices[1] & 0xF000) << 8;
        a |= (g_blocks[+rb.block].m_spriteIndices[2] & 0xF000) << 16;
        a |= (g_blocks[+rb.block].m_spriteIndices[3] & 0xF000) << 24;
        uint32 b = g_blocks[+rb.block].m_spriteIndices[4] & 0xF000;
        b |= (g_blocks[+rb.block].m_spriteIndices[5] & 0xF000) << 8;
        uvIndicies.push_back(a);
        uvIndicies.push_back(b);
    }
    sp->UpdateUniformVec3("u_position",             (GLsizei)s_blocksToDraw_opaque.size(),  (GLfloat*)positions.data());
    sp->UpdateUniformVec3("u_scale",                (GLsizei)s_blocksToDraw_opaque.size(),  (GLfloat*)scale.data());
    sp->UpdateUniformUintStream("u_spriteIndicies", (GLsizei)uvIndicies.size(),             (GLuint*)uvIndicies.data());
    positions.clear();
    scale.clear();
    uvIndicies.clear();
#else
    sp->UpdateUniformUintStream("u_renderBlockStruct", sizeof(s_blocksToDraw_opaque[0]) * s_blocksToDraw_opaque.size(), (GLuint*)s_blocksToDraw_opaque.data());
#endif

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glDisableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, (GLsizei)s_blocksToDraw_opaque.size());
    g_renderer.numTrianglesDrawn += (36 / 3) * (uint32)s_blocksToDraw_opaque.size();

    //Mat4 modelMatrix;
    //gb_mat4_translate(&modelMatrix, currentCube.p.p);
    //sp->UpdateUniformMat4("u_model", 1, false, modelMatrix.e);
#endif
    s_blocksToDraw_opaque.clear();
}





//UNTESTED AND DOES NOT WORK
void Draw2DSquare(Rect rect, Color color)
{
    std::unique_ptr<VertexBuffer> vertexBuffer = std::make_unique<VertexBuffer>();

    //WRONG COORDINATE SPACE -1 TO 1 NOT 0 TO SIZE OF SCREEN IN PIXELS
    Vertex vertices[4] = {};
    vertices[0].p = { rect.botLeft.x,  rect.topRight.y, 1.5f };
    vertices[1].p = { rect.botLeft.x,  rect.botLeft.y,  1.5f };
    vertices[2].p = { rect.topRight.x, rect.topRight.y, 1.5f };
    vertices[3].p = { rect.topRight.x, rect.botLeft.y,  1.5f };
    vertices[0].uv = { 0, 1 };
    vertices[1].uv = { 0, 0 };
    vertices[2].uv = { 1, 1 };
    vertices[3].uv = { 1, 0 };

    vertexBuffer->Upload(vertices, arrsize(vertices));

    vertexBuffer->Bind();
    uint32 indices[] = { 0, 1, 2, 1, 3, 2 };
    uint32 spriteIndices[+Face::Count] = { 0, 0, 0, 0, 0, 0 };

    std::unique_ptr<IndexBuffer> indexBuffer = std::make_unique<IndexBuffer>();
    indexBuffer->Upload(indices, arrsize(indices));
    indexBuffer->Bind();

    Mat4 perspective, view, transform, scale;
    gb_mat4_identity(&perspective);
    view, transform, scale = perspective;

    g_renderer.textures[Texture::T::Plain]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
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

    glDrawElements(GL_TRIANGLES, arrsize(indices), GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += arrsize(indices);
}
