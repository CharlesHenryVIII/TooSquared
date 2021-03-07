#include "Block.h"
#include "WinInterop.h"
#include "Noise.h"


#if SOFA == 1
ChunkArray* g_chunks;
void ChunkArray::ClearChunk(ChunkIndex index)
{
    active[index] = {};
    blocks[index] = {};
    p[index] = {};
    faceVertices[index] = {};

    //delete vertexBuffer[index];
    //TODO: FIX
    //vertexBuffer[index] = {};

    uploadedIndexCount[index] = {};
    flags[index] = {};
}

ChunkIndex ChunkArray::AddChunk()
{
    for (ChunkIndex i = 0; i < MAX_CHUNKS; i++)
    {
        if (!active[i])
        {
            //TODO: move this to the destruction of a chunk/index
            ClearChunk(i);
            active[i] = true;
            g_chunks->chunkCount++;
            return i;
        }
    }
    return uint32(-1);
}
#endif

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

#if SOFA == 1
Vec3Int ChunkArray::BlockPosition(ChunkIndex i)
{
    return { g_chunks->p[i].x * static_cast<int32>(CHUNK_X), g_chunks->p[i].y * static_cast<int32>(CHUNK_Y), g_chunks->p[i].z * static_cast<int32>(CHUNK_Z) };
}

#else
Vec3Int Chunk::BlockPosition()
{
    return { p.x * static_cast<int32>(CHUNK_X), p.y * static_cast<int32>(CHUNK_Y), p.z * static_cast<int32>(CHUNK_Z) };
}
#endif


Vec3Int ToChunkPosition(Vec3 p)
{

    Vec3Int result = { static_cast<int32>(p.x) / static_cast<int32>(CHUNK_X),
                       static_cast<int32>(p.y) / static_cast<int32>(CHUNK_Y),
                       static_cast<int32>(p.z) / static_cast<int32>(CHUNK_Z) };
    return result;
}

#if SOFA == 1
void ChunkArray::SetBlocks(ChunkIndex i)
{
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
            Vec3Int chunkBlockP = BlockPosition(i);

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
                //	bt = BlockType::Empty;
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
}
#else

void Chunk::SetBlocks()
{
	BlockType options[] = {
		BlockType::Empty,
		BlockType::Grass,
		BlockType::Dirt,
		BlockType::Stone,
		BlockType::IronBlock,
	};
	blocks = std::make_unique<ChunkData>();

	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 z = 0; z < CHUNK_Z; z++)
		{
			Vec2 blockP = { static_cast<float>(x), static_cast<float>(z) };
			Vec3Int chunkBlockP = BlockPosition();

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
				//	bt = BlockType::Empty;
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
				blocks->e[x][y][z] = bt;
			}
		}
	}

	//Layer 1 + 2
	blocks->e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
	blocks->e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
	blocks->e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 5] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
	blocks->e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 7] = BlockType::Grass;
	blocks->e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 6] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 4] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 4] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 6][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 7][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 4][CHUNK_Y - 6][CHUNK_Z - 1] = BlockType::Grass;



	//Layer 3
	blocks->e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 7] = BlockType::Grass;
	blocks->e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 6] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 4] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 4][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 4][CHUNK_Z - 1] = BlockType::Grass;



	//Layer 4 + 5
	blocks->e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 1][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 2][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
	blocks->e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 6] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 7] = BlockType::Grass;
	blocks->e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 6] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 4] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 4] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 7][CHUNK_Y - 2][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 6][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;
//
	blocks->e[CHUNK_X - 4][CHUNK_Y - 1][CHUNK_Z - 2] = BlockType::Grass;
	blocks->e[CHUNK_X - 4][CHUNK_Y - 2][CHUNK_Z - 1] = BlockType::Grass;}
#endif

Vec3Int GetBlockPosFromIndex(uint16 index)
{
    int32 blockY =  index / CHUNK_Y;
    int32 duplicateMath = (index - blockY * CHUNK_Y);
    int32 blockZ = (duplicateMath) / CHUNK_Z;
    int32 blockX =  duplicateMath - blockZ * CHUNK_Z;

    return { blockX, blockY, blockZ };
}

#if SOFA == 1
BlockType ChunkArray::GetBlock(ChunkIndex i, Vec3Int a)
{
    if (a.x >= CHUNK_X || a.y >= CHUNK_Y || a.z >= CHUNK_Z)
        return BlockType::Empty;

    return blocks[i].e[a.x][a.y][a.z];
}

#else
BlockType Chunk::GetBlock(Vec3Int a)
{
    if (a.x >= CHUNK_X || a.y >= CHUNK_Y || a.z >= CHUNK_Z)
        return BlockType::Empty;

    return blocks->e[a.x][a.y][a.z];
}
#endif

#if SOFA == 1
void ChunkArray::BuildChunkVertices(ChunkIndex i)
{
    faceVertices[i].clear();
    uploadedIndexCount[i] = 0;
    Vec3Int realP = BlockPosition(i);
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

        if (GetBlock(i, blockP + blockN + a) != BlockType::Empty)
            vert.connectedVertices += 2;
        if (GetBlock(i, blockP + blockN + b) != BlockType::Empty)
            vert.connectedVertices += 2;
        if (GetBlock(i, blockP + blockN + c) != BlockType::Empty)
            vert.connectedVertices += 2;


        vertIndex += 2;
        vertIndex = vertIndex % 8;
    }
}
#else
void Chunk::BuildChunkVertices()
{
    SetBlockSprites();
    faceVertices.clear();
    uploadedIndexCount = 0;
    Vec3Int realP = BlockPosition();
    for (int32 x = 0; x < CHUNK_X; x++)
    {
        for (int32 y = 0; y < CHUNK_Y; y++)
        {
            for (int32 z = 0; z < CHUNK_Z; z++)
            {
                for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
                {
                    BlockType currentBlockType = blocks->e[x][y][z];
                    if (currentBlockType == BlockType::Empty)
                        continue;

                    Vec3Int vf = Vec3ToVec3Int(faceNormals[faceIndex]);
                    int32 xReal = x + vf.x;
                    int32 yReal = y + vf.y;
                    int32 zReal = z + vf.z;

                    bool outOfBounds = (xReal >= CHUNK_X || yReal >= CHUNK_Y || zReal >= CHUNK_Z ||
                        xReal < 0 || yReal < 0 || zReal < 0);

                    if (outOfBounds || blocks->e[xReal][yReal][zReal] == BlockType::Empty)
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

                        faceVertices.push_back(f.a);
                        faceVertices.push_back(f.b);
                        faceVertices.push_back(f.c);
                        faceVertices.push_back(f.d);

                        uploadedIndexCount += 6;
                    }
                }
            }
        }
    }

    uint32 i = 0;
    for (Vertex_Chunk& vert : faceVertices)
    {
        Vec3Int blockN = Vec3ToVec3Int(faceNormals[vert.n]);
        Vec3Int blockP = GetBlockPosFromIndex(vert.blockIndex);

        uint8 faceIndex = vert.n;
        Vec3Int a = *(&vertexBlocksToCheck[faceIndex].e0 + (i + 0));
        Vec3Int b = *(&vertexBlocksToCheck[faceIndex].e0 + (i + 1));
        Vec3Int c = a + b;

        if (GetBlock(blockP + blockN + a) != BlockType::Empty)
            vert.connectedVertices += 2;
        if (GetBlock(blockP + blockN + b) != BlockType::Empty)
            vert.connectedVertices += 2;
        if (GetBlock(blockP + blockN + c) != BlockType::Empty)
            vert.connectedVertices += 2;


        i += 2;
        i = i % 8;
    }
}
#endif

#if SOFA == 1
void ChunkArray::UploadChunk(ChunkIndex i)
{
    vertexBuffer[i];
    vertexBuffer[i].Upload(faceVertices[i].data(), faceVertices[i].size());
    std::vector<Vertex_Chunk> faces;
    faceVertices[i].swap(faces);
    flags[i] |= CHUNK_UPLOADED;
}

#else
void Chunk::UploadChunk()
{
    vertexBuffer.Upload(faceVertices.data(), faceVertices.size());
    std::vector<Vertex_Chunk> faces;
    faceVertices.swap(faces);
    flags |= CHUNK_UPLOADED;
}
#endif

void PreChunkRender()
{
    assert(g_renderer.chunkIB);
    if (g_renderer.chunkIB)
        g_renderer.chunkIB->Bind();
    else
        return;

    g_renderer.programs[+Shader::Simple3D]->UseShader();
    g_renderer.spriteTextArray->Bind();

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    //gb_mat4_translate(&transform, Vec3IntToVec3(p));
    gb_mat4_identity(&transform);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
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

#if SOFA == 1
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

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
    sp->UpdateUniformVec3("u_chunkP",      1,  Vec3IntToVec3(BlockPosition(i)).e);

    glDrawElements(GL_TRIANGLES, (GLsizei)uploadedIndexCount[i], GL_UNSIGNED_INT, 0);
}
#else

void Chunk::RenderChunk()
{
    vertexBuffer.Bind();

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, blockIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, spriteIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, connectedVertices));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
    sp->UpdateUniformVec3("u_chunkP",      1,  Vec3IntToVec3(BlockPosition()).e);

    glDrawElements(GL_TRIANGLES, (GLsizei)uploadedIndexCount, GL_UNSIGNED_INT, 0);
}
#endif



