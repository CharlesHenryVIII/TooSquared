#include "Block.h"
#include "WinInterop.h"
#include "Noise.h"
#include "Computer.h"


ChunkArray* g_chunks;

int64 PositionHash(Vec3Int p)
{
    int64 result = {};
    result = static_cast<int64>(p.z) & 0x00000000FFFFFFFFLL;
    result |= static_cast<int64>(p.x) << (8 * sizeof(int32));
    return result;
}

bool ChunkArray::GetChunkFromPosition(ChunkIndex& result, Vec3Int p)
{
    //auto it = g_chunks->chunkPosTable.find(PositionHash(chunkP));
    //if (it != g_chunks->chunkPosTable.end())
    //{
    //    //result = index;
    //    result = it->second;
    //    return true;
    //}
    //else
    //    return false;

    auto it = chunkPosTable.find(PositionHash(p));
    if (it != chunkPosTable.end())
    {
        result = it->second;
        return true;
    }
    return false;
}

    //bool                                    active[MAX_CHUNKS];
    //ChunkData                               blocks[MAX_CHUNKS];
    //Vec3Int                                 p[MAX_CHUNKS];
    //std::vector<Vertex_Chunk>               faceVertices[MAX_CHUNKS];
    //VertexBuffer                            vertexBuffer[MAX_CHUNKS] = {};
    //uint32                                  uploadedIndexCount[MAX_CHUNKS];
    //uint16                                  flags[MAX_CHUNKS];
    //uint32                                  chunkCount = 0;
    //State                                   state[MAX_CHUNKS];
    //std::unordered_map<uint64, ChunkIndex>  chunkPosTable;

void ChunkArray::ClearChunk(ChunkIndex index)
{
    auto it = chunkPosTable.find(PositionHash(p[index]));
    assert(it != chunkPosTable.end());
    chunkPosTable.erase(it);

    active[index] = {};
    blocks[index] = {};
    p[index] = {};

    faceVertices[index].clear();
    std::vector<Vertex_Chunk> swapping;
    faceVertices[index].swap(swapping);
    state[index] = {};
    uploadedIndexCount[index] = {};
    flags[index] = {};

    g_chunks->chunkCount--;

    //delete vertexBuffer[index];
    //TODO: FIX
    //vertexBuffer[index] = {};

}

ChunkIndex ChunkArray::AddChunk(Vec3Int position)
{
    assert(OnMainThread());
    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
    {
        if (!active[i])
        {
            active[i] = true;
            g_chunks->chunkCount++;
            g_chunks->chunkPosTable[PositionHash(position)] = i;
            g_chunks->p[i] = position;
            return i;
        }
    }
    return uint32(-1);
}

struct VertexFace {
    Vertex_Chunk a,b,c,d;
};

Vec3 faceNormals[] = {

{  1.0f,  0.0f,  0.0f },
{ -1.0f,  0.0f,  0.0f },
{  0.0f,  1.0f,  0.0f },
{  0.0f, -1.0f,  0.0f },
{  0.0f,  0.0f,  1.0f },
{  0.0f,  0.0f, -1.0f },
};

union VertexBlockCheck {
    struct { Vec3Int e0, e1, e2, e3, e4, e5, e6, e7; };
};

static const VertexBlockCheck vertexBlocksToCheck[+Face::Count] = {
    {//right +X
        Vec3Int({  0,  1,  0 }),//Vertex 0
                {  0,  0,  1 },

                {  0,  0,  1 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                {  0,  0, -1 },

                {  0,  0, -1 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//left -X
        Vec3Int({  0,  1,  0 }),//Vertex 0
                {  0,  0, -1 },

                {  0,  0, -1 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                {  0,  0,  1 },

                {  0,  0,  1 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//Top +Y
        Vec3Int({  0,  0,  1 }),//Vertex 0
                {  1,  0,  0 },

                {  1,  0,  0 }, //Vertex 1
                {  0,  0, -1 },
                {  0,  0,  1 }, //Vertex 2
                { -1,  0,  0 },

                { -1,  0,  0 }, //Vertex 3
                {  0,  0, -1 },
    },
    {//Bot -Y
        Vec3Int({  0,  0,  1 }),//Vertex 0
                { -1,  0,  0 },

                { -1,  0,  0 }, //Vertex 1
                {  0,  0, -1 },
                {  0,  0,  1 }, //Vertex 2
                {  1,  0,  0 },

                {  1,  0,  0 }, //Vertex 3
                {  0,  0, -1 },
    },
    {//Front +Z
        Vec3Int({  0,  1,  0 }),//Vertex 0
                { -1,  0,  0 },

                { -1,  0,  0 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                {  1,  0,  0 },

                {  1,  0,  0 }, //Vertex 3
                {  0, -1,  0 },
    },
    {//Front -Z
        Vec3Int({  0,  1,  0 }),//Vertex 0
                {  1,  0,  0 },

                {  1,  0,  0 }, //Vertex 1
                {  0, -1,  0 },
                {  0,  1,  0 }, //Vertex 2
                { -1,  0,  0 },

                { -1,  0,  0 }, //Vertex 3
                {  0, -1,  0 },
    },
};

struct BlockSprites
{
    uint8 faceSprites[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
};

static BlockSprites faceSprites[+BlockType::Count];

void SetMultipleBlockSprites(BlockType bt, uint32 v)
{
    for (uint32 i = 0; i < +Face::Count; i++)
    {
        faceSprites[+bt].faceSprites[i] = v;
    }
}
void SetBlockSprites()
{
    SetMultipleBlockSprites(BlockType::Grass, 3);
    faceSprites[+BlockType::Grass].faceSprites[+Face::Top] = 0;
    faceSprites[+BlockType::Grass].faceSprites[+Face::Bot] = 2;

    SetMultipleBlockSprites(BlockType::Stone, 1);
    SetMultipleBlockSprites(BlockType::IronBlock, 22);
    SetMultipleBlockSprites(BlockType::Dirt, 2);
}

Vec3Int Convert_ChunkIndexToGame(ChunkIndex i)
{
    if (g_chunks->active[i])
        return { g_chunks->p[i].x * static_cast<int32>(CHUNK_X), g_chunks->p[i].y * static_cast<int32>(CHUNK_Y), g_chunks->p[i].z * static_cast<int32>(CHUNK_Z) };
    return {};
}

Vec3Int Convert_GameToChunk(Vec3 p)
{
    Vec3Int result = { static_cast<int32>(p.x) / static_cast<int32>(CHUNK_X),
                       static_cast<int32>(p.y) / static_cast<int32>(CHUNK_Y),
                       static_cast<int32>(p.z) / static_cast<int32>(CHUNK_Z) };
    return result;
}

void ChunkArray::SetBlocks(ChunkIndex i)
{
    //PROFILE_SCOPE("SetBlocks() ");
    BlockType options[] = {
        BlockType::Empty,
        BlockType::Grass,
        BlockType::Dirt,
        BlockType::Stone,
        BlockType::IronBlock,
    };

    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 z = 0; z < CHUNK_Z; z++)
        {
            Vec2 blockP = { static_cast<float>(x), static_cast<float>(z) };
            Vec3Int chunkBlockP = Convert_ChunkIndexToGame(i);

            Vec2 blockRatio = { chunkBlockP.x + blockP.x, chunkBlockP.z + blockP.y };

#if NOISETYPE == 2
            //blockRatio /= 200;
            //int32 yTotal = Max(static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 10);
            blockRatio /= 100;
            int32 yTotal = Clamp<uint32>(static_cast<int32>(Noise(blockRatio, 1.0f) * CHUNK_Y), 10, CHUNK_Y - 1);
#elif NOISETYPE == 4
            blockRatio /= 100;
            int32 yTotal = (static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 80);
#else
            static_assert(false, "Need to set noise implimentation variabls in SetBlocks()");
#endif
            assert(yTotal >= 0 && yTotal < CHUNK_Y);


            for (int32 y = 0; y < yTotal; y++)
            {

                BlockType bt = BlockType::Empty;
                //if (y > CHUNK_Y / 2)
                //{
                //  bt = BlockType::Empty;
                //}
                //else
                {
                    if (y == yTotal - 1)
                    {
                        bt = BlockType::Grass;
                    }
                    else if (y > yTotal - 4)
                    {
                        bt = BlockType::Dirt;
                        //uint32 random = RandomU32(+BlockType::Grass, static_cast<uint32>(arrsize(options)));
                    }
                    else
                    {
                        //uint32 random = RandomU32(+BlockType::Stone, static_cast<uint32>(arrsize(options)));
                        //uint32 random = RandomU32(0, static_cast<uint32>(arrsize(options)));
                        //bt = options[random];
                        bt = BlockType::Stone;
                    }
                }
                blocks[i].e[x][y][z] = bt;
            }
        }
    }

#if 0
    //Layer 1 + 2
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 5] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;



    //Layer 3
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;



    //Layer 4 + 5
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
    blocks[i].e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
#endif
}

Vec3Int GetBlockPosFromIndex(uint16 index)
{
    int32 blockY =  index / CHUNK_Y;
    int32 duplicateMath = (index - blockY * CHUNK_Y);
    int32 blockZ = (duplicateMath) / CHUNK_Z;
    int32 blockX =  duplicateMath - blockZ * CHUNK_Z;

    return { blockX, blockY, blockZ };
}

//returns false on failure and true on success/found
bool ChunkArray::GetChunk(ChunkIndex& result, Vec3Int blockP)
{
    assert(OnMainThread());
    Vec3Int chunkP = { blockP.x / static_cast<int32>(CHUNK_X), blockP.y / static_cast<int32>(CHUNK_Y), blockP.z / static_cast<int32>(CHUNK_Z) };
#if 0
    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
    {
        if ((!g_chunks->active[i]) || (g_chunks->flags[i] & CHUNK_LOADING_BLOCKS))
            continue;
        if (g_chunks->p[i] == ChunkP)
        {
            result = i;
            return true;
        }
    }
    return false;
#else
    ChunkIndex index;
    if (GetChunkFromPosition(index, chunkP))
    {
        result = index;
        return true;
    }
    else
        return false;
#endif
}

Vec3Int Convert_BlockToGame(ChunkIndex blockParentIndex, Vec3Int blockP)
{
    Vec3Int chunkLocation = Convert_ChunkIndexToGame(blockParentIndex);
    return chunkLocation + blockP;
}

bool Convert_GameToBlock(ChunkIndex& result, Vec3Int& outputP, Vec3Int inputP)
{
    if (g_chunks->GetChunk(result, inputP))
    {
        outputP = Abs(Convert_ChunkIndexToGame(result) - inputP);
        return true;
    }
    else
        return false;
}

bool ChunkArray::GetBlock(BlockType& result, ChunkIndex blockParentIndex, Vec3Int blockRelP, bool crossChunkBoundary)
{
    if (blockRelP.y >= CHUNK_Y || blockRelP.y < 0)
    {
        result = BlockType::Empty;
        return true;
    }
    else if ((blockRelP.x >= CHUNK_X || blockRelP.z >= CHUNK_Z) || (blockRelP.x < 0 ||  blockRelP.z < 0))
    {
        if (!crossChunkBoundary)
        {
            result = BlockType::Empty;
            return false;
        }
        else
        {
            Vec3Int blockSpace_block = Convert_BlockToGame(blockParentIndex, blockRelP);
            ChunkIndex newChunkIndex = {};
            Vec3Int newRelBlockP = {};
            if (Convert_GameToBlock(newChunkIndex, newRelBlockP, blockSpace_block))
            {
                result = g_chunks->blocks[newChunkIndex].e[newRelBlockP.x][newRelBlockP.y][newRelBlockP.z];
                return true;
            }
            else
            {
                result = BlockType::Empty;
                return false;
            }
        }
    }

    result = blocks[blockParentIndex].e[blockRelP.x][blockRelP.y][blockRelP.z];
    return true;
}


void ChunkArray::BuildChunkVertices(ChunkIndex i, ChunkIndex* neighbors)
{
    faceVertices[i].clear();
    faceVertices[i].reserve(10000);
    uploadedIndexCount[i] = 0;
    Vec3Int realP = Convert_ChunkIndexToGame(i);
    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 y = 0; y < CHUNK_Y; y++)
        {
            for (int32 z = 0; z < CHUNK_Z; z++)
            {
                for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
                {
                    BlockType currentBlockType = blocks[i].e[x][y][z];
                    if (currentBlockType == BlockType::Empty)
                        continue;

                    Vec3Int vf = Vec3ToVec3Int(faceNormals[faceIndex]);
                    int32 xReal = x + vf.x;
                    int32 yReal = y + vf.y;
                    int32 zReal = z + vf.z;

                    bool outOfBounds = (xReal >= CHUNK_X || yReal >= CHUNK_Y || zReal >= CHUNK_Z ||
                        xReal < 0 || yReal < 0 || zReal < 0);

                    if (outOfBounds || blocks[i].e[xReal][yReal][zReal] == BlockType::Empty)
                    {
                        VertexFace f = {};// = cubeFaces[faceIndex];
                        Vec3 offset = { static_cast<float>(x + realP.x), static_cast<float>(y + realP.y), static_cast<float>(z + realP.z) };

                        f.a.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.b.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.c.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
                        f.d.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;

                        f.a.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.b.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.c.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
                        f.d.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];

                        f.a.n = faceIndex;
                        f.b.n = faceIndex;
                        f.c.n = faceIndex;
                        f.d.n = faceIndex;

                        faceVertices[i].push_back(f.a);
                        faceVertices[i].push_back(f.b);
                        faceVertices[i].push_back(f.c);
                        faceVertices[i].push_back(f.d);

                        uploadedIndexCount[i] += 6;
                    }
                }
            }
        }
    }

    uint32 vertIndex = 0;
    for (Vertex_Chunk& vert : faceVertices[i])
    {
        Vec3Int blockN = Vec3ToVec3Int(faceNormals[vert.n]);
        Vec3Int blockP = GetBlockPosFromIndex(vert.blockIndex);

        uint8 faceIndex = vert.n;
        Vec3Int a = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 0));
        Vec3Int b = *(&vertexBlocksToCheck[faceIndex].e0 + (vertIndex + 1));
        Vec3Int c = a + b;

        BlockType at = BlockType::Empty;
        BlockType bt = BlockType::Empty;
        BlockType ct = BlockType::Empty;

        const bool checkAcrossChunkBoundary = false;
        if (GetBlock(at, i, blockP + blockN + a, checkAcrossChunkBoundary) == false || GetBlock(bt, i, blockP + blockN + b, checkAcrossChunkBoundary) == false ||
            GetBlock(ct, i, blockP + blockN + c, checkAcrossChunkBoundary) == false)
        {
            //failed to get block needs to be redone;
            g_chunks->flags[i] |= CHUNK_RESCAN_BLOCKS;
        }
        else
            g_chunks->flags[i] &= ~(CHUNK_RESCAN_BLOCKS);
        if (at != BlockType::Empty)
            vert.connectedVertices += 2;
        if (bt != BlockType::Empty)
            vert.connectedVertices += 2;
        if (ct != BlockType::Empty)
            vert.connectedVertices += 2;

        vertIndex += 2;
        vertIndex = vertIndex % 8;
    }
}

void ChunkArray::UploadChunk(ChunkIndex i)
{
    vertexBuffer[i].Upload(faceVertices[i].data(), faceVertices[i].size());
    std::vector<Vertex_Chunk> faces;
    faceVertices[i].swap(faces);
    g_chunks->state[i] = ChunkArray::Uploaded;
}

void PreChunkRender()
{
    assert(g_renderer.chunkIB);
    if (g_renderer.chunkIB)
        g_renderer.chunkIB->Bind();
    else
        return;

    g_renderer.programs[+Shader::Chunk]->UseShader();
    g_renderer.spriteTextArray->Bind();

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    //gb_mat4_translate(&transform, Vec3IntToVec3(p));
    gb_mat4_identity(&transform);

    ShaderProgram* sp = g_renderer.programs[+Shader::Chunk];
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);

#if DIRECTIONALLIGHT == 1
    sp->UpdateUniformVec3("u_directionalLight_d",  1,  g_light.d.e);
#else
    sp->UpdateUniformVec3("u_lightColor",  1,  g_light.c.e);
    sp->UpdateUniformVec3("u_lightP",      1,  g_light.p.e);
#endif
    sp->UpdateUniformVec3("u_cameraP",     1,  g_camera.p.e);

    sp->UpdateUniformUint8("u_CHUNK_X", CHUNK_X);
    sp->UpdateUniformUint8("u_CHUNK_Y", CHUNK_Y);
    sp->UpdateUniformUint8("u_CHUNK_Z", CHUNK_Z);

    Material material;
    material.ambient = { 0.02f, 0.02f, 0.02f };
    material.diffuse = { 1.0f, 1.0f, 1.0f };
    material.specular = { 0.4f, 0.4f,  0.4f };
    material.shininess = 32;//0.78125f;
    sp->UpdateUniformVec3( "material.ambient",  1,  material.ambient.e);
    sp->UpdateUniformVec3( "material.diffuse",  1,  material.diffuse.e);
    sp->UpdateUniformVec3( "material.specular", 1,  material.specular.e);
    sp->UpdateUniformFloat("material.shininess",    material.shininess);

}

void ChunkArray::RenderChunk(ChunkIndex i)
{
    vertexBuffer[i].Bind();

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, blockIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, spriteIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, connectedVertices));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);

    ShaderProgram* sp = g_renderer.programs[+Shader::Chunk];
    sp->UpdateUniformVec3("u_chunkP",      1,  Vec3IntToVec3(Convert_ChunkIndexToGame(i)).e);

    glDrawElements(GL_TRIANGLES, (GLsizei)uploadedIndexCount[i], GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += uploadedIndexCount[i] / 3;
}

void SetBlocks::DoThing()
{
    //PROFILE_SCOPE("THREAD: SetBlocks()");
    g_chunks->SetBlocks(chunk);
    g_chunks->state[chunk] = ChunkArray::BlocksLoaded;
}

void CreateVertices::DoThing()
{
    //PROFILE_SCOPE("THREAD: CreateVertices()");
    g_chunks->BuildChunkVertices(chunk, neighbors);
    g_chunks->state[chunk] = ChunkArray::VertexLoaded;
}

void DrawBlock(Vec3 p, Color color, float scale)
{
    DrawBlock(p, color, { scale, scale, scale });
}

void DrawBlock(Vec3 p, Color color, Vec3 scale)
{

    if (g_renderer.cubeVertexBuffer == nullptr)
    {
        const Vec3 cubeVertices[] = {
            // +x
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 0, 1.0),
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(1.0, 0, 0),

            // -x
            gb_vec3(0, 1.0, 0),
            gb_vec3(0, 0, 0),
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 0, 1.0),

            // +y
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 1.0, 0),

            // -y
            gb_vec3(0, 0, 1.0),
            gb_vec3(0, 0, 0),
            gb_vec3(1.0, 0, 1.0),
            gb_vec3(1.0, 0, 0),

            // z
            gb_vec3(0, 1.0, 1.0),
            gb_vec3(0, 0, 1.0),
            gb_vec3(1.0, 1.0, 1.0),
            gb_vec3(1.0, 0, 1.0),

            // -z
            gb_vec3(1.0, 1.0, 0),
            gb_vec3(1.0, 0, 0),
            gb_vec3(0, 1.0, 0),
            gb_vec3(0, 0, 0),
        };

        g_renderer.cubeVertexBuffer = new VertexBuffer();

        Vertex vertices[arrsize(cubeVertices)] = {};

        for (int32 i = 0; i < arrsize(cubeVertices); i++)
        {
            vertices[i].p = cubeVertices[i] - 0.5f;
        }

        g_renderer.cubeVertexBuffer->Upload(vertices, arrsize(vertices));
    }

    g_renderer.cubeVertexBuffer->Bind();
    g_renderer.chunkIB->Bind();

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    gb_mat4_translate(&transform, p);
    //gb_mat4_identity(&transform);

    ShaderProgram* sp = g_renderer.programs[+Shader::Cube];
    sp->UseShader();
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);
    sp->UpdateUniformVec3("u_scale",       1,        scale.e);

    sp->UpdateUniformVec4("u_color",       1,        color.e);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glDisableVertexArrayAttrib(g_renderer.vao, 1);
    glDisableVertexArrayAttrib(g_renderer.vao, 2);
    glDisableVertexArrayAttrib(g_renderer.vao, 3);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    g_renderer.numTrianglesDrawn += 36 / 3;
}
