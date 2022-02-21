#pragma once

#include "Misc.h"
#include "Rendering.h"
struct Camera;

enum class BlockType : uint8 {
    Empty,
    Grass,
    Dirt,
    Stone,
    Planks,
    StoneSlab,
    Brick,
    TNT,
    Cobblestone,
    Bedrock,
    Sand,
    Gravel,
    Wood,
    IronBlock,
    GoldBlock,
    DiamondBlock,
    Snow,
    Ice,
    Obsidian,
    Leaves,
    MossyCobblestone,
    Water,
    HalfSlab,
    Slab,
    Glass,
    Belt,
    Count,
//CobWeb,
//Flower_Red,
//Flower_Yellow,
//Flower_Blue,
//Sappling,
//Chest,
};
ENUMOPS(BlockType);

enum class Face : uint8 {
    Right,
    Left,
    Top,
    Bot,
    Back,
    Front,
    Count,
};
ENUMOPS(Face);

#define BLOCK_SEETHROUGH                BIT(1)
#define BLOCK_TRANSLUCENT               BIT(2)
#define BLOCK_COLLIDABLE                BIT(3)
#define BLOCK_SIDES_SHOULD_BE_RENDERED  BIT(4)
#define BLOCK_HAS_SHADING               BIT(5)
#define BLOCK_NON_CUBOIDAL              BIT(6)
#define BLOCK_COMPLEX                   BIT(7)

const uint32 defaultSpriteLocation = 254;
struct Block {
    uint32 m_spriteIndices[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    float m_collisionHeight = 1.0f;
    uint32 m_flags          = BLOCK_COLLIDABLE | BLOCK_HAS_SHADING;
    //Material material;
};

void Block_PlayerPlaceAction(BlockType hitBlock);
void Block_OnPlace(BlockType hitBlock);
void Block_PlayerRemoveAction(BlockType hitBlock);
void Block_OnRemove(BlockType hitBlock);

struct BlockSampler {
    BlockType blocks[+Face::Count] = {};
    GamePos m_baseBlockP = {};
    BlockType m_baseBlockType = BlockType::Empty;
    bool RegionGather(GamePos m_baseBlockP);
};

extern Block g_blocks[+BlockType::Count];



struct ComplexBlock {
    BlockType   m_type;
    Vec3Int     m_blockP = {};
    bool        m_inUse = true;

    virtual void Update(float dt, const ChunkPos& chunkPos) = 0;
    virtual void Render(const Camera* playerCamera, const ChunkPos& chunkPos) = 0;
    virtual void OnDestruct() { m_inUse = false; };
    virtual void OnConstruct() {};
};

enum class CoordinalPoint : uint8 {
    West,
    North,
    East,
    South,
    Count,
};
ENUMOPS(CoordinalPoint);
#define COMPLEX_BELT_MAX_BLOCKS_PER_BELT 2
struct Complex_Belt : ComplexBlock {
    Complex_Belt(const Complex_Belt& rhs) = delete;
    Complex_Belt& operator=(const Complex_Belt& rhs) = delete;
    Complex_Belt() = delete;
    Complex_Belt(const Vec3Int& p) { m_type = BlockType::Belt; m_blockP = p; };
    //Cube m_collider = {};
    BlockType m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT] = {};
    CoordinalPoint m_direction = CoordinalPoint::West;

    virtual void Update(float dt, const ChunkPos& chunkPos) override;
    virtual void Render(const Camera* playerCamera, const ChunkPos& chunkPos) override;
    //virtual void OnDestruct() override;
    //virtual void OnConstruct() override;
};

class ComplexBlocks {
    std::vector<ComplexBlock*> m_blocks;

    template <typename T>
    T* New(const Vec3Int& p)
    {
        T* b = new T(p);
        m_blocks.push_back(b);
        return b;
    }

public:

    ComplexBlock* GetBlock(const Vec3Int& p);
    void CleanUp();
    void Render(const Camera* playerCamera, const ChunkPos& chunkPos);
    void Update(float dt, const ChunkPos& chunkPos);
    void AddNew(const BlockType block, const Vec3Int& pos, const Vec3 forwardVector);
    void Remove(const Vec3Int& pos);
};






#define DEBUG_CUBE_RENDERER 2
//
#if DEBUG_CUBE_RENDERER == 1
struct RenderCube { //assume player camera
    WorldPos    p;
    Color       color;
    Vec3        scale;
};
#endif
void AddCubeToRender(WorldPos p, Color color, float scale);
void AddCubeToRender(WorldPos p, Color color, Vec3  scale);
void RenderTransparentCubes(Camera* playerCamera, const int32 passCount, bool lastPass);
void RenderOpaqueCubes(     Camera* playerCamera,   const int32 passCount);
//
void AddBlockToRender(WorldPos p, float scale, BlockType block);
void AddBlockToRender(WorldPos p, Vec3 scale, BlockType block);
void RenderTransparentBlocks(Camera* playerCamera,   const int32 passCount, bool lastPass);
void RenderOpaqueBlocks(     Camera* playerCamera,   const int32 passCount);
//

void BlockInit();
Rect GetUVsFromIndex(uint8 index);
void Draw2DSquare(Rect rect, Color color);
