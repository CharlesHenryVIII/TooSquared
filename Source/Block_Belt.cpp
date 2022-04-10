#include "block.h"
#include "Entity.h"
#include "Math.h"
#include "Chunk.h"

std::vector<Orientations> Complex_Belt::beltPermutations;

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

    UpdateOrientation(GetGamePos(chunkP), true);
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
Complex_Belt* GetBeltFromOffset(const BlockSampler& bs, const Vec3& sideOffset)
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
bool IsBeltFacingBelt(const Complex_Belt* belt, const Face face)
{
    Vec3 forward = { g_coordinalDirections[+belt->m_direction].x, 0, g_coordinalDirections[+belt->m_direction].y };
    if (faceNormals[+face] + forward == Vec3({}))
        return true;
    return false;
}
bool IsBeltFacingBelt(const BlockSampler& bs, const Vec3& sideOffset, const Face face)
{
    Complex_Belt* belt = GetBeltFromOffset(bs, sideOffset);
    //assert(belt);
    if (belt)
    {
        Vec3 forward = { g_coordinalDirections[+belt->m_direction].x, 0, g_coordinalDirections[+belt->m_direction].y };
        if (faceNormals[+face] + forward == Vec3({}))
            return true;
    }
    return false;
}
void Complex_Belt::UpdateOrientation(const GamePos& thisBlock, bool updateNeighbors)
{
    BlockSampler bs;
    bs.RegionGather(thisBlock, true);
    if (!(bs.blocks[+Face::Front] == BlockType::Belt || bs.blocks[+Face::Back] == BlockType::Belt ||
        bs.blocks[+Face::Left] == BlockType::Belt || bs.blocks[+Face::Right] == BlockType::Belt))
    {
        m_beltType = BeltType::Normal;
        return;
    }
    Vec3 offsets[+Face::Count] = {};
    offsets[+Face::Front]   = {  g_coordinalDirections[+m_direction].x, 0,  g_coordinalDirections[+m_direction].y };
    offsets[+Face::Left]    = {  offsets[+Face::Front].z,               0, -offsets[+Face::Front].x };
    offsets[+Face::Right]   = { -offsets[+Face::Front].z,               0,  offsets[+Face::Front].x };
    offsets[+Face::Back]    = { -offsets[+Face::Front] };

    Face relativeFaces[+Face::Count];
    Complex_Belt* belts[+Face::Count] = {};
    for (int32 i = 0; i < +Face::Count; i++)
    {
        if (!(i == +Face::Top || i == +Face::Bot))
        {
            relativeFaces[i] = GetFace(offsets[i]);
            belts[i] = GetBeltFromOffset(bs, offsets[i]);
        }
    }

    Orientations o;
    for (int32 i = 0; i < +Face::Count; i++)
    {
        o.e[i] = Orientation_None;
        if (i == +Face::Top || i == +Face::Bot || i == +Face::Front)
            continue;

        if (belts[i])
        {
            o.e[i] = IsBeltFacingBelt(belts[i], relativeFaces[i]) ? Orientation_Towards : Orientation_NotTowards;
        }
    }
    bool matchFound = false;
    for (int32 i = 0; i < beltPermutations.size(); i++)
    {
        const Orientations& p = beltPermutations[i];
        bool permutationMatch = true;
        for (int32 j = 0; j < +Face::Count; j++)
        {
            if (!((p.e[j] == o.e[j]) || (p.e[j] == Orientation_Any) || 
                (p.e[j] == Orientation_NotTowards && o.e[j] == Orientation_None) ))
            {
                permutationMatch = false;
                break;
            }
        }
        if (permutationMatch)
        {
            m_beltType = p.type;
            matchFound = true;
            break;
        }
    }
    assert(matchFound);
    if (updateNeighbors)
    {
        for (int32 i = 0; i < +Face::Count; i++)
        {
            if (belts[i])
            {
                GamePos p;
                p.p = thisBlock.p + Vec3ToVec3Int(offsets[i]);
                belts[i]->UpdateOrientation(p, false);
            }
        }
    }
}
void Complex_Belt::OnConstruct(const GamePos& thisBlock, const Vec3Int& pos, const Vec3 forwardVector)
{
    UpdateOrientation(thisBlock, true);
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

void Complex_Belt::Render(const Camera* playerCamera, const int32 passCount, const ChunkPos& chunkPos)
{
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
    WorldPos p = ToWorld(chunkPos).p + Vec3({ float(m_blockP.x), float(m_blockP.y), float(m_blockP.z) });
    RenderVoxMesh(p, 1.0f, playerCamera, mesh, modelIndex, CoordinalPointToRad(m_direction), passCount);

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
