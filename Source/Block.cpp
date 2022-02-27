#include "Block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"

Block g_blocks[+BlockType::Count] = {};

const Vec2 faceUV[4] = {
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
    g_blocks[+BlockType::Empty].m_flags |= BLOCK_SEETHROUGH | BLOCK_TRANSLUCENT | BLOCK_NON_CUBOIDAL;
    g_blocks[+BlockType::Empty].m_flags &= ~(BLOCK_COLLIDABLE | BLOCK_HAS_SHADING);

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
    g_blocks[+BlockType::Ice].m_flags |= BLOCK_SEETHROUGH | BLOCK_TRANSLUCENT;
    SetMultipleBlockSprites(BlockType::Obsidian, 37);
#if 1
    SetMultipleBlockSprites(BlockType::Leaves, 52);
    g_blocks[+BlockType::Leaves].m_flags |= BLOCK_SEETHROUGH | BLOCK_SIDES_SHOULD_BE_RENDERED;
#else
    SetMultipleBlockSprites(BlockType::Leaves, 53);
#endif
    SetMultipleBlockSprites(BlockType::MossyCobblestone, 36);
    SetMultipleBlockSprites(BlockType::TNT, 8);
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Top] = 9;
    g_blocks[+BlockType::TNT].m_spriteIndices[+Face::Bot] = 10;
    SetMultipleBlockSprites(BlockType::Water, 255);
    g_blocks[+BlockType::Water].m_flags |= BLOCK_SEETHROUGH | BLOCK_TRANSLUCENT;
    g_blocks[+BlockType::Water].m_flags &= ~(BLOCK_COLLIDABLE);
    SetMultipleBlockSprites(BlockType::Bedrock, 17);

    SetMultipleBlockSprites(BlockType::HalfSlab, 5);
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::HalfSlab].m_spriteIndices[+Face::Bot] = 6;
    g_blocks[+BlockType::HalfSlab].m_size.y = 0.5f;
    g_blocks[+BlockType::HalfSlab].m_flags |= BLOCK_SEETHROUGH;

    SetMultipleBlockSprites(BlockType::Slab, 5);
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Top] = 6;
    g_blocks[+BlockType::Slab].m_spriteIndices[+Face::Bot] = 6;

    SetMultipleBlockSprites(BlockType::Glass, 49);
    g_blocks[+BlockType::Glass].m_flags |= BLOCK_SEETHROUGH | BLOCK_TRANSLUCENT;
    g_blocks[+BlockType::Glass].m_flags &= ~(BLOCK_HAS_SHADING);

    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Left]     = 138;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Right]    = 138;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Bot]      = 139;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Front]    = 154;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Back]     = 154;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Top]      = 155;
    g_blocks[+BlockType::Belt].m_flags |= BLOCK_SEETHROUGH | BLOCK_NON_CUBOIDAL | BLOCK_COMPLEX | BLOCK_INTERACT;
    g_blocks[+BlockType::Belt].m_size.y = 0.5f;
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


CoordinalPoint ForwardVectorToCoordinalPoint(const Vec3& forward)
{
    CoordinalPoint result = {};
    if (forward.x == 0.0f && forward.z == 0.0f)
        return result;
    float rad = atan2f(forward.x, forward.z);
    float deg = RadToDeg(rad);
    deg += 180;                 //normalize negative values to be 0 to 360
    deg = deg / 90.0f;          //Get deg in terms of quarter rotations
    deg += 0.5;                 //Round to the nearest quarter
    int32 coord = (int32)deg;   //Truncate
    coord = coord % (+CoordinalPoint::Count);
    result = (CoordinalPoint)coord;
    return result;
}


//
// Complex Blocks
//
void ComplexBlocks::AddNew(const BlockType block, const Vec3Int& pos, const Vec3 forwardVector)
{
    switch (block)
    {
    case BlockType::Belt:
    {
        auto* complex = New<Complex_Belt>(pos);
        if ((forwardVector.x == forwardVector.y) && (forwardVector.z == 0.0f) && (forwardVector.z == forwardVector.x))
            complex->m_direction = CoordinalPoint::West;
        else
            complex->m_direction = ForwardVectorToCoordinalPoint(forwardVector);
        complex->OnConstruct();
        break;
    }
    default:
    {
        assert(false);
        break;
    }
    }

}
ComplexBlock* ComplexBlocks::GetBlock(const Vec3Int& p)
{
    for (int32 i = 0; i < m_blocks.size(); i++)
    {
        if (m_blocks[i]->m_blockP == p)
            return m_blocks[i];
    }
    return nullptr;
}
void ComplexBlocks::Remove(const Vec3Int& p)
{
    ComplexBlock* block = GetBlock(p);
    if (block == nullptr)
        return;
    block->OnDestruct();
    block->m_inUse = false;
}
void ComplexBlocks::Render(const Camera* playerCamera, const ChunkPos& chunkPos)
{
    ComplexBlock* b = nullptr;
    for (int32 i = 0; i < m_blocks.size(); i++)
    {
        b = m_blocks[i];
        if (b->m_inUse)
            b->Render(playerCamera, chunkPos);
    }
}
void ComplexBlocks::Update(float dt, const ChunkPos& chunkPos)
{
    for (int32 i = 0; i < m_blocks.size(); i++)
    {
        m_blocks[i]->Update(dt, chunkPos);
    }
}
void ComplexBlocks::CleanUp()
{
    std::erase_if(m_blocks,
        [](const ComplexBlock* b)
        {
            return (!(b->m_inUse));
        });
}
static const std::string complexBlocksSaveDir = "\\Block_Data";
bool ComplexBlocksInit()
{
    bool success = true;
    std::string filename = g_gameData.m_folderPath;
    success &= CreateFolder(filename);
    filename = g_gameData.m_saveFolderPath;
    success &= CreateFolder(filename);
    filename += g_gameData.m_saveFilename;
    success &= CreateFolder(filename);
    filename += complexBlocksSaveDir;
    success &= CreateFolder(filename);
    return success;
}
struct ComplexBlocksHeader {
    uint32 m_header;
    uint32 m_type;
    uint32 m_version;
};
struct Complex_BeltData {
    Vec3Int         m_p;
    CoordinalPoint  m_direction;
};
void ComplexBlocks::Save(const ChunkPos& p)
{
    ComplexBlocksHeader mainHeader = {};
    mainHeader.m_header  = SDL_FOURCC('B', 'L', 'O', 'C');
    mainHeader.m_type    = SDL_FOURCC('C', 'P', 'L', 'X');
    mainHeader.m_version = 1;
    std::string fullFileName = GetComplexBlockSaveFilePathFromChunkPos(p);
    assert(fullFileName.size() > 0);
    File file = File(fullFileName, File::Mode::Write, true);
    file.Write(&mainHeader, sizeof(ComplexBlocksHeader));
    
    for (int32 i = 0; i < m_blocks.size(); i++)
    {
        auto& block = m_blocks[i];
        block->Save(&file);
    }
}
bool ComplexBlocks::Load(const ChunkPos& p)
{
    ComplexBlocksHeader mainHeaderRef = {};
    mainHeaderRef.m_header  = SDL_FOURCC('B', 'L', 'O', 'C');
    mainHeaderRef.m_type    = SDL_FOURCC('C', 'P', 'L', 'X');
    mainHeaderRef.m_version = 1;

    std::string fullFilePath = GetComplexBlockSaveFilePathFromChunkPos(p);
    File file(fullFilePath, File::Mode::Read, false);
    bool success = true;
    if (file.m_handleIsValid)
    {
        file.GetData();
        if (file.m_binaryDataIsValid)
        {
            ComplexBlocksHeader* mainHeader = (ComplexBlocksHeader*)file.m_dataBinary.data();
            if (mainHeader->m_header == mainHeaderRef.m_header && mainHeader->m_type == mainHeaderRef.m_type && mainHeader->m_version == mainHeaderRef.m_version)
            {
                size_t count = (file.m_dataBinary.size() - sizeof(ComplexBlocksHeader)) / sizeof(Complex_BeltData);
                Complex_BeltData* blockStart = (Complex_BeltData*)(mainHeader + 1);
                for (size_t i = 0; i < count; i++)
                {
                    Complex_BeltData cbData = (Complex_BeltData)blockStart[i];
                    Complex_Belt* cb = New<Complex_Belt>(cbData.m_p);
                    cb->m_direction = cbData.m_direction;
                }
            }
        }
    }
    else
        success = false;
    return success;
}

WorldPos ComplexBlock::GetWorldPos(const ChunkPos& chunkPos) const
{
    WorldPos result;
    result.p = ToWorld(chunkPos).p;
    result.p += Vec3({ float(m_blockP.x), float(m_blockP.y), float(m_blockP.z) });
    return result;
}


//
// Complex Block Belt
//
void Complex_Belt::Update(float dt, const ChunkPos& chunkPos)
{
    for (int32 i = 0; i < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; i++)
    {
        const BlockType childType = m_blocks[i].m_type;
        if (childType != BlockType::Empty)
        {
            WorldPos childBlockSize;
            childBlockSize.p = HadamardProduct(g_blocks[+childType].m_size, g_itemScale);

            WorldPos p = GetWorldPos(chunkPos);
            p.p.y += g_blocks[+BlockType::Belt].m_size.y;
            p.p.y += childBlockSize.p.y / 2;//size of miniature blocks
            AddBlockToRender(p, g_itemScale, childType);
        }
    }

#if 0
    rotationTime += dt;
    if (rotationTime >= 1.0f)
    {
        (*(uint8*)&m_direction)++;
        rotationTime = 0;
    }
    if (+m_direction >= +CoordinalPoint::Count)
        m_direction = {};
#endif
}
void Complex_Belt::Render(const Camera* playerCamera, const ChunkPos& chunkPos)
{
#if 1
    //WorldPos pos = ToWorld(chunkPos).p;
    //pos.p += { float(m_blockP.x), float(m_blockP.y), float(m_blockP.z) };
    const int32 vertexCount = 4 * +Face::Count;
    Vertex_Complex vertices[vertexCount];
    for (int32 f = 0; f < +Face::Count; f++)
    {
        for (int32 i = 0; i < 4; i++)
        {
            Vertex_Complex& v = vertices[f * 4 + i];
            v.p = smallCubeVertices[f].e[i];
            if (f == +Face::Top || f == +Face::Bot)
                v.uv = faceUV[i];
            else
            {
                v.uv = faceUV[i];
                v.uv.y = v.uv.y / 2;
            }
            v.n = faceNormals[f];
            v.i = g_blocks[+m_type].m_spriteIndices[f];
        }
    }


    VertexBuffer vertexBuffer = VertexBuffer();
    vertexBuffer.Upload(vertices, vertexCount);
    vertexBuffer.Bind();
    g_renderer.chunkIB->Bind();
    //glactiveTexture(GL_TEXTURE0);
    //g_renderer.spriteTextArray->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::BlockComplex];

    sp->UseShader();
    Mat4 rot;
    gb_mat4_rotate(&rot, {0, 1, 0}, float(m_direction) * (tau / 4.0f));
    sp->UpdateUniformMat4("u_rotate", 1, false, rot.e);
    Mat4 translate;
    gb_mat4_translate(&translate, { -0.5, 0, -0.5 });
    sp->UpdateUniformMat4("u_toModel", 1, false, translate.e);
    Mat4 translateBack;
    gb_mat4_translate(&translateBack, { 0.5, 0, 0.5 });
    sp->UpdateUniformMat4("u_fromModel", 1, false, translateBack.e);
    Mat4 model;
    gb_mat4_identity(&model);
    gb_mat4_translate(&model, ToWorld(chunkPos).p + Vec3({ float(m_blockP.x), float(m_blockP.y), float(m_blockP.z) }));
    sp->UpdateUniformMat4("u_model", 1, false, model.e);
    sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
    sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);
    sp->UpdateUniformUint8("u_passCount", 0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Complex), (void*)offsetof(Vertex_Complex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_Complex), (void*)offsetof(Vertex_Complex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Complex), (void*)offsetof(Vertex_Complex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Complex), (void*)offsetof(Vertex_Complex, i));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36;
#else
    GamePos chunkLocation = ToGame(chunkPos);
    GamePos blockLocation = { chunkLocation.p.x + m_p.x, chunkLocation.p.y + m_p.y, chunkLocation.p.z + m_p.z };
    AddBlockToRender(ToWorld(blockLocation), 1.0f, m_type);
#endif
}
bool Complex_Belt::Save(File* file)
{
    bool success = true;
    Complex_BeltData cbData;
    cbData.m_p = m_blockP;
    cbData.m_direction = m_direction;
    success &= file->Write(&cbData, sizeof(Complex_BeltData));
    return success;
}

bool Complex_Belt::OnInteract(const BlockType& blockType, uint8& count)
{
    bool result = false;
    if (count)
    {
        if (m_blocks[0].m_type == BlockType::Empty && m_blocks[1].m_type == BlockType::Empty)
        {
            m_blocks[0].m_type = blockType;
            m_blocks[0].m_position = 0.0f;
            --count;
            result = true;
        }
    }
    return result;
}

void Block_PlayerPlaceAction(BlockType hitBlock)
{
    switch (hitBlock)
    {
    case BlockType::Belt:
    {

    }
    default:
    {

    }
    }
}
void Block_OnPlace(BlockType hitBlock)
{

}
void Block_PlayerRemoveAction(BlockType hitBlock)
{

}
void Block_OnRemove(BlockType hitBlock)
{

}




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

std::vector<Vertex_Block> s_blocksToDraw_transparent;
std::vector<Vertex_Block> s_blocksToDraw_opaque;
void AddBlockToRender(WorldPos p, float scale, BlockType block)
{
    AddBlockToRender(p, { scale, scale, scale }, block);
}
void AddBlockToRender(WorldPos p, Vec3 scale, BlockType block)
{
    Vertex_Block b = {};
    b.p = p.p;
    b.scale = scale;
    auto* list = &s_blocksToDraw_opaque;
    if (g_blocks[+block].m_flags & BLOCK_TRANSLUCENT)
        list = &s_blocksToDraw_transparent;
    for (int32 f = 0; f < +Face::Count; f++)
    {
        b.index = g_blocks[+block].m_spriteIndices[f];
        for (int32 v = 0; v < 4; v++)
            list->push_back(b);
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
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Block), (void*)offsetof(Vertex_Block, index));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);
    {
        ZoneScopedN("Render Blocks");
        glDrawElements(GL_TRIANGLES, (GLsizei)((blocksToDraw->size() / 24) * 36), GL_UNSIGNED_INT, 0);
    }
    g_renderer.numTrianglesDrawn += 12 * (uint32)blocksToDraw->size();

    if (clearBlocksToDraw)
        blocksToDraw->clear();
}
void RenderTransparentBlocks(Camera* playerCamera, const int32 passCount, bool lastPass)
{
    ZoneScopedN("Upload and Render Transparent Blocks");
    RenderBlocksInternal(playerCamera, passCount, &s_blocksToDraw_transparent, lastPass);
}
void RenderOpaqueBlocks(Camera* playerCamera, const int32 passCount)
{
    ZoneScopedN("Upload and Render Opaque Blocks");
    RenderBlocksInternal(playerCamera, passCount, &s_blocksToDraw_opaque, true);
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
