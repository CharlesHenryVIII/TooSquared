#include "block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"


const Vec2 s_coordinalDirections[] = {
    {  0, -1 },
    {  1,  0 },
    {  0,  1 },
    { -1,  0 },
};

const Vec2 faceUV[4] = {
    Vec2{ 0, 1 },
    Vec2{ 0, 0 },
    Vec2{ 1, 1 },
    Vec2{ 1, 0 }
};


//
//  Belt Common
//
void Complex_Belt::OnDestruct(const ChunkPos& chunkP)
{
    WorldPos worldPos = GetWorldPos(chunkP);
    for (int32 i = 0; i < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; i++)
    {
        if (m_blocks[i].m_type != BlockType::Empty)
        {
            ChunkIndex chunkIndex;
            if (g_chunks->GetChunk(chunkIndex, ToGame(chunkP)))
            {
#if 1
                WorldPos blockWorldP = GetChildBlockPos(i, worldPos);
#else
                WorldPos blockWorldP = GetWorldPos(chunkP);
                WorldPos dest = blockWorldP.p;
                dest.p.y += 1.0f;
#endif

                g_items.Add(g_chunks->itemIDs[chunkIndex], m_blocks[i].m_type, blockWorldP);
                m_blockCount--;
            }
        }
    }
}
Complex_Belt* Complex_Belt::GetNextBelt(const ChunkPos& chunkPos)
{
    GamePos nextBlockP = GetGamePos(chunkPos);
    nextBlockP.p += Vec3Int(int32(-1 * s_coordinalDirections[+m_direction].x), 0, int32(s_coordinalDirections[+m_direction].y));
    //fetch next block
    BlockType nextBlock;
    ChunkIndex nextChunkIndex;
    if (g_chunks->GetBlock(nextBlock, nextBlockP, nextChunkIndex) && nextBlock == BlockType::Belt)
    {
        ChunkPos c;
        Vec3Int nextBlockPos = Convert_GameToBlock(c, nextBlockP);
        ComplexBlock* cb = g_chunks->complexBlocks[nextChunkIndex].GetBlock(nextBlockPos);
        Complex_Belt* belt = (Complex_Belt*)cb;
        return belt;
    }
    return nullptr;
}
void Complex_Belt::RemoveFinalBlock()
{
    for (int32 i = COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 2; i >= 0; i--)
    {
        m_blocks[i + 1] = m_blocks[i];
    }
    m_blocks[0] = {};
    m_blockCount--;
    if (m_blockCount == 0)
        m_running = false;
}
void Complex_Belt::OnHover()
{
    int32 index;
    if (CanAddBlock_Offset(index, BlockType::Stone))
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


        std::string interactText = "'F' To Add Block";
        if (ImGui::Begin(interactText.c_str(), nullptr, windowFlags))
        {
            ImGui::Text(interactText.c_str());
        }
        ImGui::End();
    }
}


//
// Complex Block Belt
//
WorldPos Complex_Belt::GetChildBlockPos(const int32 index, const WorldPos& parentPos)
{
    WorldPos p = parentPos;
    float& distance = m_blocks[index].m_position;

    //Find the X and Z of the child block
    switch (m_type)
    {
    case BlockType::Belt:
    case BlockType::Belt_UpVert:
    case BlockType::Belt_DownVert:
    {
#if 0
        Vec4 childBlockRelativeToParent = {};
        childBlockRelativeToParent.x = distance;
        childBlockRelativeToParent.z = 0.5f;
        childBlockRelativeToParent.w = 1.0f;
        float rads = (1.0f - (float(+m_direction) / +CoordinalPoint::Count)) * tau;
        Mat4 rot;
        gb_mat4_rotate(&rot, { 0, 1, 0 }, rads);
        Vec4 finalPosition = rot * childBlockRelativeToParent;
#else

        if (s_coordinalDirections[+m_direction].x)
        {
            p.p.z += (g_blocks[+BlockType::Belt].m_size.z / 2.0f);
            float amount = distance;
            if (s_coordinalDirections[+m_direction].x > 0)
                amount = 1 - amount;
            p.p.x += amount;
        }
        else
        {
            p.p.x += (g_blocks[+BlockType::Belt].m_size.x / 2.0f);
            float amount = distance;
            if (s_coordinalDirections[+m_direction].y < 0)
                amount = 1 - amount;
            p.p.z += amount;
        }
#endif
        break;
    }
    case BlockType::Belt_Turn:
    {
        assert(false);
    }
    }

    //Find the Y of the child block
    WorldPos childBlockSize;
    childBlockSize.p = HadamardProduct(g_blocks[+m_blocks[index].m_type].m_size, g_itemScale);
    switch (m_type)
    {
    case BlockType::Belt_Turn:
    case BlockType::Belt:
    {
        p.p.y += g_blocks[+BlockType::Belt].m_size.y;
        p.p.y += childBlockSize.p.y / 2;//size of miniature blocks
        break;
    }
    case BlockType::Belt_UpVert:
    {
        //p.p.y += g_blocks[+BlockType::Belt].m_size.y;
        break;
    }
    case BlockType::Belt_DownVert:
    {
        //p.p.y += g_blocks[+BlockType::Belt].m_size.y;
        break;
    }
    }
    return p;
}

void Complex_Belt::Update(float dt, const ChunkPos& chunkPos)
{
    const WorldPos worldP = GetWorldPos(chunkPos);
    const Complex_Belt_Child_Block& finalBlock = m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1];
    assert(m_blockCount >= 0);
    assert(m_blockCount >= 0 && m_blockCount <= COMPLEX_BELT_MAX_BLOCKS_PER_BELT);
    assert((m_blockCount > 0) == (finalBlock.m_type != BlockType::Empty));
    if (!m_blockCount)
    { 
        assert(m_running == false);
        m_running = false;
        return;
    }

    //Determine if the belt should be runnning
    //Useless????
    float maxDistance = 0.0f;
    bool nextBeltHasSpace = false;
    Complex_Belt* nextBelt = nullptr;
    nextBelt = GetNextBelt(chunkPos);
    int32 index;
    if (nextBelt && nextBelt->CanAddBlock_Front(index, finalBlock.m_type))
    {
        //there is a next belt
        maxDistance = 1.0f;
        nextBeltHasSpace = true;
    }
    else
    {
        maxDistance = g_blocks[+BlockType::Belt].m_size.x - (g_itemScale.x * g_blocks[+finalBlock.m_type].m_size.x) / 2.0f;
    }
    if (!m_running)
    {
        if (finalBlock.m_position < maxDistance)
            m_running = true;
        else if (finalBlock.m_position > maxDistance)
        {
            assert(false); //am I ever hitting this?
            ChunkIndex chunkIndex;
            WorldPos p = GetChildBlockPos(COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1, worldP);
            assert(g_chunks->GetChunk(chunkIndex, ToGame(p)));
            WorldPos d = p;
            d.p.y += 1.0f;
            g_items.Add(g_chunks->itemIDs[chunkIndex], finalBlock.m_type, p, d);
            RemoveFinalBlock();
        }
    }

    if (m_running)
        m_beltPosition = fmodf(m_beltPosition + m_beltSpeed * dt, 1.0f);

    for (int32 i = 0; i < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; i++)
    {
        const BlockType childType = m_blocks[i].m_type;
        float& distance = m_blocks[i].m_position;
        if (childType != BlockType::Empty)
        {
            if (m_running)
            {
                distance += (m_beltSpeed * dt);
                if (distance >= maxDistance)
                {
                    assert(i == COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1);
                    if (nextBeltHasSpace && m_blockCount > 0)
                    {
                        //m_running = true;
                        WorldPos p = GetChildBlockPos(i, worldP);
                        AddBlockToRender(p, g_itemScale, childType);
                        
                        if (!nextBelt->AddBlock_Front(childType))
                        {
                            ChunkIndex chunkIndex;
                            assert(g_chunks->GetChunk(chunkIndex, ToGame(p)));
                            g_items.Add(g_chunks->itemIDs[chunkIndex], childType, p);
                        }
                        assert(COMPLEX_BELT_MAX_BLOCKS_PER_BELT >= 2);
                        RemoveFinalBlock();
                        
                        break;
                    }
                    else
                    {
                        float diff = distance - maxDistance;
                        if (diff > m_beltSpeed * dt)
                        {
                            ChunkIndex chunkIndex;
                            WorldPos p = GetChildBlockPos(COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1, worldP);
                            assert(g_chunks->GetChunk(chunkIndex, ToGame(p)));
                            WorldPos d = p;
                            d.p.y += 1.0f;
                            g_items.Add(g_chunks->itemIDs[chunkIndex], finalBlock.m_type, p, d);
                            RemoveFinalBlock();
                        }
                        for (int32 j = 0; j < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; j++)
                        {
                            if (m_blocks[j].m_type != BlockType::Empty)
                            {
                                m_blocks[j].m_position -= diff;
                            }
                            m_beltPosition -= diff;
                        }
                        m_running = false;
                    }
                }
            }

            WorldPos p = GetChildBlockPos(i, worldP);
            AddBlockToRender(p, g_itemScale, childType);
        }
    }

    //debug rotation code
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
        if (f == +Face::Top)
        {
            for (int32 i = 0; i < 4; i++)
            {
                Vertex_Complex& v = vertices[f * 4 + i];
                v.p  = smallCubeVertices[f].e[i];
                v.uv = faceUV[i];
                v.uv.y += m_beltPosition;
                v.n  = faceNormals[f];
                v.i  = g_blocks[+m_type].m_spriteIndices[f];
            }
        }
        else
        {
            for (int32 i = 0; i < 4; i++)
            {
                Vertex_Complex& v = vertices[f * 4 + i];
                v.p = smallCubeVertices[f].e[i];
                if (f == +Face::Bot)
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
    Complex_BeltData_V3 cbData = {};
    cbData.m_p              = m_blockP;
    cbData.m_direction      = m_direction;
    cbData.m_beltType       = m_beltType;
    cbData.m_beltPosition   = m_beltPosition;
    cbData.m_running        = m_running;
    for (int32 i = 0; i < COMPLEX_BELT_MAX_BLOCKS_PER_BELT; i++)
        cbData.m_blocks[i] = m_blocks[i];
    success &= file->Write(&cbData, sizeof(Complex_BeltData_V3));
    return success;
}

bool Complex_Belt::AddBlock_Front(BlockType child)
{
    int32 index;
    if (CanAddBlock_Front(index, child))
    {
        m_blocks[index].m_type = child;
        m_blocks[index].m_position = 0;
        m_blockCount++;
        return true;
    }
    return false;
}

bool Complex_Belt::AddBlock_Offset(BlockType child)
{
    int32 index;
    if (CanAddBlock_Offset(index, child))
    {
        m_blocks[index].m_type = child;
        m_blocks[index].m_position = (g_itemScale.x * g_blocks[+child].m_size.x) / 2;
        m_blockCount++;
        return true;
    }
    return false;
}

bool Complex_Belt::CanAddBlock_Front(int32& index, const BlockType child) const
{
    for (int32 i = COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1; i >= 0; i--)
    {
        if (m_blocks[i].m_type == BlockType::Empty)
        {
            if ((i + 1) < COMPLEX_BELT_MAX_BLOCKS_PER_BELT)
            {
                // TODO: Impliment rototion into calculation so longer blocks in one dimension will fill the belt
                float offset = (g_itemScale.x * g_blocks[+m_blocks[i + 1].m_type].m_size.x) / 2;
                if (m_blocks[i + 1].m_position > offset)
                {
                    index = i;
                    return true;
                }
            }
            else
            {
                index = i;
                return true;
            }
        }
    }
    return false;
}

bool Complex_Belt::CanAddBlock_Offset(int32& index, const BlockType child) const
{
    for (int32 i = COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1; i >= 0; i--)
    {
        if (m_blocks[i].m_type == BlockType::Empty)
        {
            // TODO: Impliment rototion into calculation so longer blocks in one dimension will fill the belt
            float newBlockSize          = (g_itemScale.x * g_blocks[+child].m_size.x);
            float currentBlockOffset    = (g_itemScale.x * g_blocks[+m_blocks[i].m_type].m_size.x) / 2.0f;
            if (i + 1 < COMPLEX_BELT_MAX_BLOCKS_PER_BELT)
            {
                if (m_blocks[i + 1].m_position >= (newBlockSize + currentBlockOffset)) 
                {
                    index = i;
                    return true;
                }
            }
            else
            {
                index = i;
                return true;
            }
        }
    }
    return false;
}
