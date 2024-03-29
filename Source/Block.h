#pragma once

#include "Math.h"
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

const Vec2 g_coordinalDirections[] = {
    {  0, -1 }, //North
    {  1,  0 }, //East
    {  0,  1 }, //South
    { -1,  0 }, //West
};


#define BLOCK_SEETHROUGH                BIT(1)
#define BLOCK_TRANSLUCENT               BIT(2)
#define BLOCK_COLLIDABLE                BIT(3)
#define BLOCK_SIDES_SHOULD_BE_RENDERED  BIT(4)
#define BLOCK_HAS_SHADING               BIT(5)
#define BLOCK_NON_CUBOIDAL              BIT(6)
#define BLOCK_COMPLEX                   BIT(7)
#define BLOCK_INTERACT                  BIT(8)

#define VOXELS_PER_BLOCK    16

const uint32 defaultSpriteLocation = 254;
struct Block {
    uint32 m_spriteIndices[+Face::Count] = {
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
        defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
    uint32 m_flags = BLOCK_COLLIDABLE | BLOCK_HAS_SHADING;
    Vec3   m_size  = { 1.0f, 1.0f, 1.0f };
};

struct BlockSampler {
    BlockType blocks[+Face::Count] = {};
    GamePos   m_baseBlockP = {};
    BlockType m_baseBlockType = BlockType::Empty;
    bool RegionGather(GamePos m_baseBlockP, bool baseBlockEmpty = false);
};

extern Block g_blocks[+BlockType::Count];
enum OrientationType : uint32 {
    Orientation_Any         = 0,
    Orientation_None        = BIT(0),
    Orientation_Towards     = BIT(1),
    Orientation_NotTowards  = BIT(2),
#if 0
    Orientation_RightNone       = BIT( 0),
    Orientation_RightTowards    = BIT( 1),
    Orientation_RightNotTowards = BIT( 2),
    Orientation_LeftNone        = BIT( 3),
    Orientation_LeftTowards     = BIT( 4),
    Orientation_LeftNotTowards  = BIT( 5),
    Orientation_BackNone        = BIT( 6),
    Orientation_BackTowards     = BIT( 7),
    Orientation_BackNotTowards  = BIT( 8),
    Orientation_FrontNone       = BIT( 9),
    Orientation_FrontTowards    = BIT(10),
    Orientation_FrontNotTowards = BIT(11),
#endif
};


struct Inventory;
struct ComplexBlock {
    BlockType   m_type;
    Vec3Int     m_blockP = {};
    bool        m_inUse = true;

    WorldPos GetWorldPos(const ChunkPos& chunkPos) const;
    GamePos GetGamePos(const ChunkPos& chunkPos) const;
    virtual void Update(float dt, const ChunkPos& chunkPos) = 0;
    virtual void Render(const Camera* playerCamera, const int32 passCount, const ChunkPos& chunkPos) = 0;
    virtual bool Save(File* file) { return true; };
    //virtual bool OnInteract(Inventory& inventory) { return true; };
    virtual void OnHover();
    virtual bool CanAddBlock_Front( int32& index, const BlockType child) const { return false; };
    virtual bool CanAddBlock_Offset(int32& index, const BlockType child) const { return false; };
    virtual bool AddBlock_Front( BlockType child) { return false; };
    virtual bool AddBlock_Offset(BlockType child) { return false; };
    virtual void OnConstruct(const GamePos& hitBlock, const Vec3Int& pos, const Vec3 forwardVector) {};
    virtual void OnDestruct(const ChunkPos& chunkP) { m_inUse = false; };
};

enum class CoordinalPoint : uint8 {
    North,
    East,
    South,
    West,
    Count,
};
ENUMOPS(CoordinalPoint);
enum class BeltType : uint8 {
    Error,
    Normal,
    Turn_CCW,
    Turn_CW,
    Up1,
    Up2,
    Down1,
    Down2,
    Count,
};
ENUMOPS(BeltType);
struct Orientations
{
    OrientationType e[+Face::Count];
    BeltType type;
};
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

    static std::vector<Orientations> beltPermutations;
    Complex_Belt_Child_Block m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT] = {};
    CoordinalPoint  m_direction = CoordinalPoint::West;
    float           m_beltSpeed = 0.1f; //units per second, 0.1 would take 10 seconds to go 1 block
    float           rotationTime = 0;
    int32           m_blockCount = 0;
    bool            m_running = false;
    float           m_beltPosition = 0.0f;
    BeltType        m_beltType = BeltType::Normal;


    void UpdateOrientation(const GamePos& thisBlock, bool updateNeighbors);
    virtual void Update(float dt, const ChunkPos& chunkPos) override;
    virtual void OnDestruct(const ChunkPos& chunkP) override;
    virtual void OnConstruct(const GamePos& hitBlock, const Vec3Int& pos, const Vec3 forwardVector) override;
    virtual void Render(const Camera* playerCamera, const int32 passCount, const ChunkPos& chunkPos);
    virtual bool Save(File* file) override;
    virtual void OnHover() override;
    virtual bool AddBlock_Front(BlockType child) override;
    virtual bool AddBlock_Offset(BlockType child) override;
    virtual bool CanAddBlock_Front (int32& index, const BlockType child) const override;
    virtual bool CanAddBlock_Offset(int32& index, const BlockType child) const override;
    WorldPos GetChildBlockPos(const int32 index, const WorldPos& parentPos, bool voxelStep);
    Complex_Belt* GetNextBelt(const ChunkPos& chunkPos);

protected:
    void RemoveFinalBlock();
};

float CoordinalPointToRad(const CoordinalPoint point);
CoordinalPoint ForwardVectorToCoordinalPoint(const Vec3& forward);



struct Complex_BeltData {
    Vec3Int         m_p;
    CoordinalPoint  m_direction;
};
#pragma pack(push, 1)
struct Complex_BeltData_V2 {
    Vec3Int         m_p = {};
    CoordinalPoint  m_direction = {};
    Complex_Belt_Child_Block m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT] = {};
};
struct Complex_BeltData_V3 {
    Vec3Int         m_p = {};
    CoordinalPoint  m_direction = {};
    BeltType        m_beltType = {};
    Complex_Belt_Child_Block m_blocks[COMPLEX_BELT_MAX_BLOCKS_PER_BELT] = {};
    float           m_beltPosition;
    bool            m_running;
};
#pragma pack(pop)


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
    void Render(const Camera* playerCamera, const int32 passCount, const ChunkPos& chunkPos);
    void Update(float dt, const ChunkPos& chunkPos);
    void AddNew(const GamePos& hitBlock, const BlockType block, const Vec3Int& pos, const Vec3 forwardVector);
    void Remove(const ChunkPos& chunkP, const Vec3Int& pos);
    bool Load(const ChunkPos& p);
    void Save(const ChunkPos& p);
};

bool ComplexBlocksInit();





void AddCubeToRender(WorldPos p, Color color, float scale);
void AddCubeToRender(WorldPos p, Color color, Vec3  scale);
void RenderTransparentCubes(Camera* playerCamera, const int32 passCount, bool lastPass);
void RenderOpaqueCubes(     Camera* playerCamera,   const int32 passCount);
//
void AddBlockToRender(const WorldPos& p, const float scale, const BlockType block, const Color& c = White, const Vec3& forward = {});
void AddBlockToRender(const WorldPos& p, const Vec3& scale, const BlockType block, const Color& c = White, const Vec3& forward = {});
void RenderTransparentBlocks(Camera* playerCamera,   const int32 passCount, bool lastPass);
void RenderOpaqueBlocks(     Camera* playerCamera,   const int32 passCount);
//

void BlockInit();
Rect GetUVsFromIndex(uint8 index);
void Draw2DSquare(Rect rect, Color color);
