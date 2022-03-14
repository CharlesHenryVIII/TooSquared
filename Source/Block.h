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
#define BLOCK_INTERACT                  BIT(8)

const uint32 defaultSpriteLocation = 254;
struct Block {
    uint32 m_spriteIndices[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    uint32 m_flags          = BLOCK_COLLIDABLE | BLOCK_HAS_SHADING;
    Vec3 m_size = { 1.0f, 1.0f, 1.0f };

    //Material material;
};

struct BlockSampler {
    BlockType blocks[+Face::Count] = {};
    GamePos m_baseBlockP = {};
    BlockType m_baseBlockType = BlockType::Empty;
    bool RegionGather(GamePos m_baseBlockP);
};

extern Block g_blocks[+BlockType::Count];


struct Inventory;
struct ComplexBlock {
    BlockType   m_type;
    Vec3Int     m_blockP = {};
    bool        m_inUse = true;

    WorldPos GetWorldPos(const ChunkPos& chunkPos) const;
    GamePos GetGamePos(const ChunkPos& chunkPos) const;
    virtual void Update(float dt, const ChunkPos& chunkPos) = 0;
    virtual void Render(const Camera* playerCamera, const ChunkPos& chunkPos) = 0;
    virtual bool Save(File* file) { return true; };
    //virtual bool OnInteract(Inventory& inventory) { return true; };
    virtual void OnHover();
    virtual bool CanAddBlock_Front( int32& index, const BlockType child) const { return false; };
    virtual bool CanAddBlock_Offset(int32& index, const BlockType child) const { return false; };
    virtual bool AddBlock_Front( BlockType child) { return false; };
    virtual bool AddBlock_Offset(BlockType child) { return false; };
    virtual void OnConstruct() {};
    virtual void OnDestruct(const ChunkPos& chunkP) { m_inUse = false; };
};

enum class CoordinalPoint : uint8 {
    West,
    North,
    East,
    South,
    Count,
};
ENUMOPS(CoordinalPoint);
#pragma pack(push, 1)
struct Complex_Belt_Child_Block {
    BlockType m_type = BlockType::Empty;
    float m_position;
};
#pragma pack(pop)
#define COMPLEX_BELT_MAX_BLOCKS_PER_BELT 2
struct Complex_Belt : ComplexBlock {
    Complex_Belt(const Complex_Belt& rhs) = delete;
    Complex_Belt& operator=(const Complex_Belt& rhs) = delete;
    Complex_Belt() = delete;
    Complex_Belt(const Vec3Int& p) { m_type = BlockType::Belt; m_blockP = p; };

private:
    void RemoveFinalBlock();

public:
    Complex_Belt_Child_Block m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT] = {};
    CoordinalPoint m_direction = CoordinalPoint::West;
    float m_beltSpeed = 0.1f; //units per second, 0.1 would take 10 seconds to go 1 block
    float rotationTime = 0;
    int32 m_blockCount = 0;
    bool m_running = false;
    float m_beltPosition = 0.0f;

    virtual void Update(float dt, const ChunkPos& chunkPos) override;
    //bool OnInteract(Inventory& inventory) override;
    virtual void OnDestruct(const ChunkPos& chunkP) override;
    virtual void Render(const Camera* playerCamera, const ChunkPos& chunkPos) override;
    virtual bool Save(File* file);
    virtual void OnHover();
    virtual bool AddBlock_Front (BlockType child);
    virtual bool AddBlock_Offset(BlockType child);
    virtual bool CanAddBlock_Front (int32& index, const BlockType child) const;
    virtual bool CanAddBlock_Offset(int32& index, const BlockType child) const;
    WorldPos GetChildBlockPos(const int32 index, const WorldPos& parentPos);
    Complex_Belt* GetNextBelt(const ChunkPos& chunkPos);
};

class ComplexBlocks {

    template <typename T>
    T* New(const Vec3Int& p)
    {
        T* b = new T(p);
        m_blocks.push_back(b);
        return b;
    }

public:

    std::vector<ComplexBlock*> m_blocks;
    ComplexBlock* GetBlock(const Vec3Int& p);
    void CleanUp();
    void Render(const Camera* playerCamera, const ChunkPos& chunkPos);
    void Update(float dt, const ChunkPos& chunkPos);
    void AddNew(const BlockType block, const Vec3Int& pos, const Vec3 forwardVector);
    void Remove(const ChunkPos& chunkP, const Vec3Int& pos);
    bool Load(const ChunkPos& p);
    void Save(const ChunkPos& p);
};

bool ComplexBlocksInit();





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
