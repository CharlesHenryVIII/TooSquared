#include "block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"


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
                WorldPos blockWorldP = GetChildBlockPos(i, worldPos, false);
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
Face GetFace(const Vec3& v)
{
    Face f;
    if (faceNormals[+Face::Front] == v)
        f = Face::Front;
    else if (faceNormals[+Face::Back] == v)
        f = Face::Back;
    else if (faceNormals[+Face::Left] == v)
        f = Face::Left;
    else if (faceNormals[+Face::Right] == v)
        f = Face::Right;
    else
        assert(false);
    return f;
}
Complex_Belt* GetBeltFromOffset(const BlockSampler& bs, const Vec3& sideOffset, const Face face)
{
    GamePos blockGameP;
    blockGameP.p = bs.m_baseBlockP.p + Vec3ToVec3Int(sideOffset);
    ChunkIndex chunkIndex;
    g_chunks->GetChunk(chunkIndex, blockGameP);
    ChunkPos chunkP;
    Vec3Int blockP = Convert_GameToBlock(chunkP, blockGameP);
    assert(g_chunks->p[chunkIndex].p == chunkP.p);
    return (Complex_Belt*)g_chunks->complexBlocks[chunkIndex].GetBlock(blockP);
}
bool IsBeltFacingBelt(const BlockSampler& bs, const Vec3& sideOffset, const Face face)
{
    Complex_Belt* belt = GetBeltFromOffset(bs, sideOffset, face);
    assert(belt);
    if (belt)
    {
        Vec3 forward = { g_coordinalDirections[+belt->m_direction].x, 0, g_coordinalDirections[+belt->m_direction].y };
        if (faceNormals[+face] + forward == Vec3({}))
            return true;
    }
    return false;
}
void Complex_Belt::OnConstruct(const GamePos& thisBlock, const Vec3Int& pos, const Vec3 forwardVector)
{
    BlockSampler bs;
    bs.RegionGather(thisBlock, true);
    if (bs.blocks[+Face::Front] == BlockType::Belt || bs.blocks[+Face::Back] == BlockType::Belt ||
        bs.blocks[+Face::Left] == BlockType::Belt || bs.blocks[+Face::Right] == BlockType::Belt)
    {
        const Vec3 forward = { g_coordinalDirections[+m_direction].x, 0, g_coordinalDirections[+m_direction].y };
        const Vec3 leftVecOffset = { forward.z, 0, -forward.x };
        const Vec3 rightVecOffset = { -forward.z, 0, forward.x };
        const Face leftFace = GetFace(leftVecOffset);
        const Face rightFace = GetFace(rightVecOffset);
        const Face frontFace = GetFace(forward);

        if (bs.blocks[+leftFace] == BlockType::Belt && bs.blocks[+rightFace] == BlockType::Belt)
        {
            bool leftIsFacingNewBelt = IsBeltFacingBelt(bs, leftVecOffset, leftFace);
            bool rightIsFacingNewBelt = IsBeltFacingBelt(bs, rightVecOffset, rightFace);
            if (leftIsFacingNewBelt != rightIsFacingNewBelt)
            {
                if (leftIsFacingNewBelt)
                {
                    m_beltType = BeltType::Turn_CCW;
                }
                else
                {
                    m_beltType = BeltType::Turn_CW;
                }
            }
        }
        else if (bs.blocks[+leftFace] == BlockType::Belt)
        {
            if (IsBeltFacingBelt(bs, leftVecOffset, leftFace))
                m_beltType = BeltType::Turn_CCW;
        }
        else if (bs.blocks[+rightFace] == BlockType::Belt)
        {
            if (IsBeltFacingBelt(bs, rightVecOffset, rightFace))
                m_beltType = BeltType::Turn_CW;
        }

        if (bs.blocks[+frontFace] == BlockType::Belt)
        {
            Complex_Belt* frontBelt = GetBeltFromOffset(bs, forward, frontFace);
            assert(frontBelt);
            if (frontBelt)
            {
                Vec3 frontForward = { g_coordinalDirections[+frontBelt->m_direction].x, 0, g_coordinalDirections[+frontBelt->m_direction].y };
                if (frontForward == leftVecOffset)
                    frontBelt->m_beltType = BeltType::Turn_CCW;
                else if (frontForward == rightVecOffset)
                    frontBelt->m_beltType = BeltType::Turn_CW;
            }
        }
    }
}
Complex_Belt* Complex_Belt::GetNextBelt(const ChunkPos& chunkPos)
{
    GamePos nextBlockP = GetGamePos(chunkPos);
    nextBlockP.p += Vec3Int(int32(g_coordinalDirections[+m_direction].x), 0, int32(g_coordinalDirections[+m_direction].y));
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

float CoordinalPointToRad(const CoordinalPoint point)
{
    int32 multiplier = ((!!(+point)) * +CoordinalPoint::Count) - +point;
    float result = float(multiplier) * (tau / 4.0f);
    return result;
}

//
// Complex Block Belt
//
WorldPos Complex_Belt::GetChildBlockPos(const int32 index, const WorldPos& parentPos, bool voxelStep)
{
    WorldPos p = parentPos;
    float distance = 0;
    if (voxelStep)
    {
        float origin = m_beltPosition - m_blocks[index].m_position;
        int32 originDiv = int32(origin * VOXELS_PER_BLOCK);
        int32 totalDiv  = int32(m_beltPosition * VOXELS_PER_BLOCK);
        distance  = (totalDiv - originDiv) / float(VOXELS_PER_BLOCK);
    }
    else
        distance = m_blocks[index].m_position;

    //Find the X and Z of the child block
    switch (m_beltType)
    {
    case BeltType::Normal:
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

        if (g_coordinalDirections[+m_direction].x)
        {
            p.p.z += (g_blocks[+BlockType::Belt].m_size.z / 2.0f);
            float amount = distance;
            if (g_coordinalDirections[+m_direction].x < 0)
                amount = 1 - amount;
            p.p.x += amount;
        }
        else
        {
            p.p.x += (g_blocks[+BlockType::Belt].m_size.x / 2.0f);
            float amount = distance;
            if (g_coordinalDirections[+m_direction].y < 0)
                amount = 1 - amount;
            p.p.z += amount;
        }
#endif
        break;
    }
    case BeltType::Turn_CCW:
    case BeltType::Turn_CW:
    {
        const Vec3 forward = { g_coordinalDirections[+m_direction].x, 0, g_coordinalDirections[+m_direction].y };
        float theta = distance * (tau / 4.0f); 
        Vec4 rotVec = { 0, 0, 0, 1.0f };
        rotVec.x = 0.5f * cosf(theta);
        rotVec.z = 0.5f * sinf(theta);
        Mat4 rot;
        gb_mat4_rotate(&rot, { 0,1,0 }, CoordinalPointToRad(m_direction));
        Vec3 vecOffset;
        Vec4 rotOffset;
        if (m_beltType == BeltType::Turn_CCW)
        {
            vecOffset = 0.5f * Vec3({ forward.z, 0, -forward.x });
            rotOffset = { rotVec.z, 0.0f, rotVec.x, 1.0f };
        }
        else
        {
            vecOffset = 0.5f * Vec3({ -forward.z, 0, forward.x });
            rotOffset = { -rotVec.z, 0.0f, rotVec.x, 1.0f };
        }
        p.p += Vec3({ 0.5f, 0, 0.5f }) + vecOffset;
        p.p += 0.5f * forward;
        Vec4 rotation = rot * rotOffset;
        p.p += rotation.xyz;
        break;
    }
    default:
        assert(false);
        break;
    }

    //Find the Y of the child block
    WorldPos childBlockSize;
    childBlockSize.p = HadamardProduct(g_blocks[+m_blocks[index].m_type].m_size, g_itemScale);
    switch (m_beltType)
    {
    case BeltType::Turn_CCW:
    case BeltType::Turn_CW:
    case BeltType::Normal:
    {
        p.p.y += g_blocks[+BlockType::Belt].m_size.y;
        p.p.y += childBlockSize.p.y / 2;//size of miniature blocks
        break;
    }
    default:
        assert(false);
        break;
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
    if (nextBelt && nextBelt->CanAddBlock_Front(index, finalBlock.m_type) && (+nextBelt->m_direction + 2 != +m_direction))
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
            WorldPos p = GetChildBlockPos(COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1, worldP, false);
            bool assertValue = g_chunks->GetChunk(chunkIndex, ToGame(p));
            assert(assertValue);
            WorldPos d = p;
            d.p.y += 1.0f;
            g_items.Add(g_chunks->itemIDs[chunkIndex], finalBlock.m_type, p, d);
            RemoveFinalBlock();
        }
    }

    if (m_running)
        m_beltPosition = fmodf(m_beltPosition + m_beltSpeed * dt, 16.0f);

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
                        WorldPos p = GetChildBlockPos(i, worldP, false);
                        AddBlockToRender(p, g_itemScale, childType);
                        
                        if (!nextBelt->AddBlock_Front(childType))
                        {
                            ChunkIndex chunkIndex;
                            bool assertValue = g_chunks->GetChunk(chunkIndex, ToGame(p));
                            assert(assertValue);
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
                            WorldPos p = GetChildBlockPos(COMPLEX_BELT_MAX_BLOCKS_PER_BELT - 1, worldP, false);
                            bool assertValue = g_chunks->GetChunk(chunkIndex, ToGame(p));
                            assert(assertValue);
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

            WorldPos p = GetChildBlockPos(i, worldP, true);
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
    Mesh mesh = Mesh::Invalid;
    switch (m_beltType)
    {
    case BeltType::Normal:
        mesh = Mesh::Belt_Normal;
        break;
    case BeltType::Turn_CCW:
        mesh = Mesh::Belt_Turn_CCW;
        break;
    case BeltType::Turn_CW:
        mesh = Mesh::Belt_Turn_CW;
        break;
    default:
        assert(false);
        mesh = Mesh::Belt_Normal;
    }
    assert(g_renderer.meshVertexBuffers[+mesh].size());
    int32 modelIndex = int32((m_beltPosition * float(VOXEL_MAX_SIZE))) % g_renderer.meshVertexBuffers[+mesh].size();
    modelIndex = Min<int32>(modelIndex, int32(g_renderer.meshVertexBuffers[+mesh].size() - 1));
    g_renderer.meshVertexBuffers[+mesh][modelIndex]->Bind();
    g_renderer.meshIndexBuffers[+mesh][modelIndex]->Bind();
    ShaderProgram* sp = g_renderer.programs[+Shader::Voxel];

    sp->UseShader();
    //Mat4 rot2;
    //gb_mat4_rotate(&rot2, {1, 0, 0}, tau / 4.0f * 3.0f);
    //sp->UpdateUniformMat4("u_modelRotate", 1, false, rot2.e);
    //Mat4 rotMov;
    //gb_mat4_translate(&rotMov, {0, 0, 1});
    //sp->UpdateUniformMat4("u_modelMove", 1, false, rotMov.e);
    Mat4 rot;
    gb_mat4_rotate(&rot, {0, 1, 0}, CoordinalPointToRad(m_direction));
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
    sp->UpdateUniformMat4( "u_model", 1, false, model.e);
    sp->UpdateUniformMat4( "u_perspective", 1, false, playerCamera->m_perspective.e);
    sp->UpdateUniformMat4( "u_view", 1, false, playerCamera->m_view.e);
    sp->UpdateUniformUint8("u_passCount", 0);
    sp->UpdateUniformVec3( "u_ambientLight",            1,  g_ambientLight.e);
    sp->UpdateUniformVec3( "u_directionalLight_d",      1,  g_renderer.sunLight.d.e);
    sp->UpdateUniformVec3( "u_lightColor",              1,  g_renderer.sunLight.c.e);
    sp->UpdateUniformVec3( "u_directionalLightMoon_d",  1,  g_renderer.moonLight.d.e);
    sp->UpdateUniformVec3( "u_moonColor",               1,  g_renderer.moonLight.c.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, ao));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, rgba.r));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, (GLsizei)g_renderer.meshIndexBuffers[+mesh][modelIndex]->m_count, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += (uint32)g_renderer.meshIndexBuffers[+mesh][modelIndex]->m_count / 3;
#else
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
