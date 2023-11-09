#include "Block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"

Block g_blocks[+BlockType::Count] = {};

struct ComplexBlocksHeader {
    uint32 m_header;
    uint32 m_type;
    uint32 m_version;
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

    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Right]    = 138;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Left]     = 138;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Top]      = 155;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Bot]      = 139;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Back]     = 154;
    g_blocks[+BlockType::Belt].m_spriteIndices[+Face::Front]    = 154;
    g_blocks[+BlockType::Belt].m_flags |= BLOCK_SEETHROUGH | BLOCK_NON_CUBOIDAL | BLOCK_COMPLEX | BLOCK_INTERACT;
    g_blocks[+BlockType::Belt].m_size.y = 0.5f;
    g_blocks[+BlockType::Belt] = g_blocks[+BlockType::Belt];
}

void BlockInit()
{
    SetBlockSprites();

    Orientations o;
    o = {};
    o.type              = BeltType::Turn_CCW;
    o.e[+Face::Right]   = Orientation_NotTowards;
    o.e[+Face::Left]    = Orientation_Towards;
    o.e[+Face::Top]     = Orientation_Any;
    o.e[+Face::Bot]     = Orientation_Any;
    o.e[+Face::Back]    = Orientation_NotTowards;
    o.e[+Face::Front]   = Orientation_Any;
    Complex_Belt::beltPermutations.push_back(o);

    o = {};
    o.type              = BeltType::Turn_CW;
    o.e[+Face::Right]   = Orientation_Towards;
    o.e[+Face::Left]    = Orientation_NotTowards;
    o.e[+Face::Top]     = Orientation_Any;
    o.e[+Face::Bot]     = Orientation_Any;
    o.e[+Face::Back]    = Orientation_NotTowards;
    o.e[+Face::Front]   = Orientation_Any;
    Complex_Belt::beltPermutations.push_back(o);

    o = {};
    o.type              = BeltType::Normal;
    o.e[+Face::Right]   = Orientation_Towards;
    o.e[+Face::Left]    = Orientation_Towards;
    o.e[+Face::Top]     = Orientation_Any;
    o.e[+Face::Bot]     = Orientation_Any;
    o.e[+Face::Back]    = Orientation_Any;
    o.e[+Face::Front]   = Orientation_Any;
    Complex_Belt::beltPermutations.push_back(o);

    o = {};
    o.type              = BeltType::Normal;
    o.e[+Face::Right]   = Orientation_NotTowards;
    o.e[+Face::Left]    = Orientation_NotTowards;
    o.e[+Face::Top]     = Orientation_Any;
    o.e[+Face::Bot]     = Orientation_Any;
    o.e[+Face::Back]    = Orientation_Any;
    o.e[+Face::Front]   = Orientation_Any;
    Complex_Belt::beltPermutations.push_back(o);

    o = {};
    o.type              = BeltType::Normal;
    o.e[+Face::Right]   = Orientation_Any;
    o.e[+Face::Left]    = Orientation_Any;
    o.e[+Face::Top]     = Orientation_Any;
    o.e[+Face::Bot]     = Orientation_Any;
    o.e[+Face::Back]    = Orientation_Towards;
    o.e[+Face::Front]   = Orientation_Any;
    Complex_Belt::beltPermutations.push_back(o);
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
#if 1
    if (fabsf(forward.x) > fabsf(forward.z))
    {
        if (forward.x > 0) result = CoordinalPoint::East;
        else result = CoordinalPoint::West;
    }
    else
    {
        if (forward.z > 0) result = CoordinalPoint::South;
        else result = CoordinalPoint::North;
    }
#else
    if (forward.x == 0.0f && forward.z == 0.0f)
        return result;
    float rad = atan2f(forward.x, forward.z);
    float deg = RadToDeg(rad);
    deg *= -1;                  //Inverse the direction
    deg += 180;                 //normalize negative values to be 0 to 360
    deg = deg / 90.0f;          //Get deg in terms of quarter rotations
    deg += 0.5;                 //Round to the nearest quarter
    int32 coord = (int32)deg;   //Truncate
    coord = coord % (+CoordinalPoint::Count);
    result = (CoordinalPoint)coord;
#endif
    return result;
}




//
//Complex Block
//
WorldPos ComplexBlock::GetWorldPos(const ChunkPos& chunkPos) const
{
    WorldPos result;
    result.p = ToWorld(chunkPos).p;
    result.p += Vec3({ float(m_blockP.x), float(m_blockP.y), float(m_blockP.z) });
    return result;
}
GamePos ComplexBlock::GetGamePos(const ChunkPos& chunkPos) const
{
    GamePos result;
    result.p = ToGame(chunkPos).p;
    result.p += m_blockP;
    return result;
}
void ComplexBlock::OnHover()
{
    const float PAD = 0.0f;
    ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos;
    window_pos.x = viewport->WorkSize.x / 2 + PAD;
    window_pos.y = viewport->WorkSize.y / 2 + PAD - 30;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, { 0.5f, 0.5f });

    ImGui::SetNextWindowBgAlpha(0.25f);
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;


    std::string interactText = "'F' To Interact";
    if (ImGui::Begin(interactText.c_str(), nullptr, windowFlags))
    {
        ImGui::Text(interactText.c_str());
    }
    ImGui::End();
}





//
// Complex Blocks
//
void ComplexBlocks::AddNew(const GamePos& hitBlock, const BlockType block, const Vec3Int& pos, const Vec3 forwardVector)
{
    switch (block)
    {
    case BlockType::Belt:
    {
        auto* complex = New<Complex_Belt>(pos);
        complex->m_type = block;
        if ((forwardVector.x == forwardVector.y) && (forwardVector.z == 0.0f) && (forwardVector.z == forwardVector.x))
            complex->m_direction = CoordinalPoint::North;
        else
            complex->m_direction = ForwardVectorToCoordinalPoint(forwardVector);
        complex->OnConstruct(hitBlock, pos, forwardVector);
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
        if (m_blocks[i]->m_blockP == p && m_blocks[i]->m_inUse)
            return m_blocks[i];
    }
    return nullptr;
}
void ComplexBlocks::Remove(const ChunkPos& chunkP, const Vec3Int& p)
{
    ComplexBlock* block = GetBlock(p);
    if (block == nullptr)
        return;
    block->m_inUse = false;
    block->OnDestruct(chunkP);
}
void ComplexBlocks::Render(const Camera* playerCamera, const int32 passCount, const ChunkPos& chunkPos)
{
    ComplexBlock* b = nullptr;
    for (int32 i = 0; i < m_blocks.size(); i++)
    {
        b = m_blocks[i];
        if (b->m_inUse)
            b->Render(playerCamera, passCount, chunkPos);
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
        [](ComplexBlock* b)
        {
            if (!b->m_inUse)
            {
                delete b;
                return true;
            }
            return false;
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

void ConvertComplexBlockSaveFile(std::string filePath, uint32 fromVersion, uint32 toVersion)
{
    for (uint32 currentVersion = fromVersion; currentVersion < toVersion; currentVersion++)
    {
        {
            File readFile(filePath, File::Mode::Read, false);
            if (readFile.m_handleIsValid)
            {
                readFile.GetData();
                if (readFile.m_binaryDataIsValid)
                {
                    File writeFile(filePath + ".part", File::Mode::Write, true);
                    writeFile.Write(readFile.m_dataBinary.data(), readFile.m_dataBinary.size());
                }
            }
        }
        File readFile(filePath + ".part", File::Mode::Read, true);
        if (readFile.m_handleIsValid)
        {
            readFile.GetData();
            if (readFile.m_binaryDataIsValid)
            {
                File writeFile(filePath, File::Mode::Write, true);
                ComplexBlocksHeader mainHeaderRef = {};
                mainHeaderRef.m_header = SDL_FOURCC('B', 'L', 'O', 'C');
                mainHeaderRef.m_type = SDL_FOURCC('C', 'P', 'L', 'X');
                mainHeaderRef.m_version = currentVersion + 1;
                writeFile.Write(&mainHeaderRef, sizeof(ComplexBlocksHeader));
                ComplexBlocksHeader* mainHeader = (ComplexBlocksHeader*)readFile.m_dataBinary.data();

                switch (currentVersion) {
                case 1: {

                    size_t count = (readFile.m_dataBinary.size() - sizeof(ComplexBlocksHeader)) / sizeof(Complex_BeltData);
                    Complex_BeltData* blockStart = (Complex_BeltData*)(mainHeader + 1);
                    Complex_BeltData from;
                    Complex_BeltData_V2 to = {};
                    for (size_t i = 0; i < count; i++)
                    {
                        from = blockStart[i];
                        to = {};
                        to.m_direction = from.m_direction;
                        to.m_p = from.m_p;
                        to.m_blocks;
                        writeFile.Write(&to, sizeof(Complex_BeltData_V2));
                    }
                    break;
                }
                case 2: {

                    size_t count = (readFile.m_dataBinary.size() - sizeof(ComplexBlocksHeader)) / sizeof(Complex_BeltData_V2);
                    Complex_BeltData_V2* blockStart = (Complex_BeltData_V2*)(mainHeader + 1);
                    Complex_BeltData_V2 from;
                    Complex_BeltData_V3 to = {};
                    for (size_t i = 0; i < count; i++)
                    {
                        from = blockStart[i];
                        to = {};
                        to.m_direction = from.m_direction;
                        to.m_p = from.m_p;
                        to.m_blocks;
                        to.m_beltType = BeltType::Normal;
                        to.m_beltPosition = 0.0f;
                        to.m_running = false;
                        writeFile.Write(&to, sizeof(Complex_BeltData_V3));
                    }
                    break;
                }
                default: assert(false);
                }
            }
            readFile.Delete();
        }
    }
}

void ComplexBlocks::Save(const ChunkPos& p)
{
    ComplexBlocksHeader mainHeader = {};
    mainHeader.m_header  = SDL_FOURCC('B', 'L', 'O', 'C');
    mainHeader.m_type    = SDL_FOURCC('C', 'P', 'L', 'X');
    mainHeader.m_version = 3;
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
    mainHeaderRef.m_version = 3;

    std::string fullFilePath = GetComplexBlockSaveFilePathFromChunkPos(p);
    bool success = false;
    bool needsReload = true;
    uint32 versionDiscrepency = 0;
    while (needsReload)
    {
        {
            File file(fullFilePath, File::Mode::Read, false);
            if (file.m_handleIsValid)
            {
                file.GetData();
                if (file.m_binaryDataIsValid)
                {
                    ComplexBlocksHeader* mainHeader = (ComplexBlocksHeader*)file.m_dataBinary.data();
                    if (mainHeader->m_header == mainHeaderRef.m_header && mainHeader->m_type == mainHeaderRef.m_type)
                    {
                        if (mainHeader->m_version == mainHeaderRef.m_version)
                        {
                            size_t count = (file.m_dataBinary.size() - sizeof(ComplexBlocksHeader)) / sizeof(Complex_BeltData_V3);
                            Complex_BeltData_V3* blockStart = (Complex_BeltData_V3*)(mainHeader + 1);
                            for (size_t i = 0; i < count; i++)
                            {
                                Complex_BeltData_V3 cbData = (Complex_BeltData_V3)blockStart[i];
                                Complex_Belt* cb = New<Complex_Belt>(cbData.m_p);
                                cb->m_direction = cbData.m_direction;
                                cb->m_beltType = cbData.m_beltType;
                                cb->m_running = cbData.m_running;
                                cb->m_beltPosition = cbData.m_beltPosition;
                                cb->m_blockCount = 0;
                                for (int32 i = 0; i < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; i++)
                                {
                                    cb->m_blocks[i] = cbData.m_blocks[i];
                                    cb->m_blockCount += (int32(cb->m_blocks[i].m_type != BlockType::Empty)); //fun
                                }
                            }
                            success = true;
                            needsReload = false;
                        }
                        else
                        {
                            versionDiscrepency = mainHeaderRef.m_version - mainHeader->m_version;
                        }
                    }
                }
            }
            else
            {
                needsReload = false;
            }
        }
        if (!success && versionDiscrepency)
        {
            ConvertComplexBlockSaveFile(fullFilePath, mainHeaderRef.m_version - versionDiscrepency, mainHeaderRef.m_version);
            needsReload = true;
        }
    }

    return success;
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
