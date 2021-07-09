#include "Block.h"
#include "Entity.h"

Block g_blocks[+BlockType::Count] = {};

static const Vec3 cubeVertices[] = {
    // +x
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    // -x
    gb_vec3(0.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 1.0f),
    // +y
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 1.0f, 0.0f),
    // -y
    gb_vec3(0.0f, 0.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    // z
    gb_vec3(0.0f, 1.0f, 1.0f),
    gb_vec3(0.0f, 0.0f, 1.0f),
    gb_vec3(1.0f, 1.0f, 1.0f),
    gb_vec3(1.0f, 0.0f, 1.0f),
    // -z
    gb_vec3(1.0f, 1.0f, 0.0f),
    gb_vec3(1.0f, 0.0f, 0.0f),
    gb_vec3(0.0f, 1.0f, 0.0f),
    gb_vec3(0.0f, 0.0f, 0.0f),
};

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
    g_blocks[+BlockType::Empty].m_transparent = true;
    g_blocks[+BlockType::Empty].m_collidable  = false;

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
    g_blocks[+BlockType::Wood].m_transparent = true;
    SetMultipleBlockSprites(BlockType::Obsidian, 37);
    SetMultipleBlockSprites(BlockType::Leaves, 53);
    //SetMultipleBlockSprites(BlockType::Leaves, 52);
    SetMultipleBlockSprites(BlockType::MossyCobblestone, 36);
    SetMultipleBlockSprites(BlockType::TNT, 8);
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Top] = 9;
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Bot] = 10;
    SetMultipleBlockSprites(BlockType::Water, 255);
    g_blocks[+BlockType::Water].m_transparent = true;
    g_blocks[+BlockType::Water].m_collidable  = false;
    SetMultipleBlockSprites(BlockType::Bedrock, 17);

    SetMultipleBlockSprites(BlockType::HalfSlab, 5);
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Bot] = 6;
    g_blocks[+BlockType::HalfSlab].m_collisionHeight           = 0.5f;
    g_blocks[+BlockType::HalfSlab].m_unUsualShape              = true;

    SetMultipleBlockSprites(BlockType::Slab, 5);
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Bot] = 6;
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

void DrawBlock(const Mat4& mat, Color color, float scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    DrawBlock(mat, color, { scale, scale, scale }, camera, textureType, blockType);
}

void DrawBlock(const Mat4& mat, Color color, Vec3 scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    std::unique_ptr<VertexBuffer> vb = std::make_unique<VertexBuffer>();

    Vertex vertices[arrsize(cubeVertices)] = {};

    for (int32 i = 0; i < arrsize(cubeVertices); i++)
    {
        vertices[i].p = cubeVertices[i] - 0.5f;
        auto spriteIndex = g_blocks[+blockType].m_spriteIndices[i / 4];
        //TODO: Refactor this garbago:
        Rect UVSquare = GetUVsFromIndex(spriteIndex);
        vertices[i].uv.x = Lerp(UVSquare.botLeft.x, UVSquare.topRight.x, faceUV[i % 4].x);
        vertices[i].uv.y = Lerp(UVSquare.topRight.y, UVSquare.botLeft.y, faceUV[i % 4].y);
    }

    vb->Upload(vertices, arrsize(vertices));
    g_renderer.chunkIB->Bind();

    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    g_renderer.textures[textureType]->Bind();
    sp->UpdateUniformMat4("u_perspective", 1, false, camera->m_perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, camera->m_view.e);
    sp->UpdateUniformMat4("u_model",       1, false, mat.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
}

void DrawCube(WorldPos p, Color color, float scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    DrawCube(p, color, { scale, scale, scale }, camera, textureType, blockType);
}
void DrawCube(WorldPos p, Color color, Vec3  scale, Camera* camera, Texture::T textureType, BlockType blockType)
{
    if (g_renderer.cubeVertexBuffer == nullptr)
    {
        g_renderer.cubeVertexBuffer = new VertexBuffer();

        Vertex vertices[arrsize(cubeVertices)] = {};
        Vec2 uvOffset = { 1.0f, 1.0f };

        for (int32 i = 0; i < arrsize(cubeVertices); i++)
        {
            vertices[i].p  = cubeVertices[i] - 0.5f;
            vertices[i].uv = faceUV[i % 4];
        }

        g_renderer.cubeVertexBuffer->Upload(vertices, arrsize(vertices));
    }

    g_renderer.cubeVertexBuffer->Bind();
    g_renderer.chunkIB->Bind();

    Mat4 transform;
    gb_mat4_translate(&transform, { p.p.x, p.p.y, p.p.z });
    g_renderer.textures[textureType]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, camera->m_perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, camera->m_view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);
    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
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