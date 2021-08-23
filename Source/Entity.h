#pragma once

#include "Math.h"
#include "Misc.h"
#include "Input.h"
#include "Inventory.h"
#include "Collision.h"

#include <vector>
#include <mutex>

enum class MovementType {
    Fly,
    Collision,
};

enum class EntityType {
    Player,
    Camera,
    Item,
};

//Vec3 g_forwardVector = { 0.0f, 0.0f, -1.0f };
//extern Vec4 g_forwardVectorRotation;
//extern Vec4 g_forwardVectorPosition;

typedef uint32 EntityID;
//static std::atomic<EntityID> s_currentEntityID = 0;
static EntityID s_currentEntityID = 0;
struct Entity
{
    Transform m_transform = {};
    EntityID m_ID = (++s_currentEntityID);
    EntityID m_parent = 0;
    std::vector<EntityID> m_children;

    bool inUse = true;
    virtual void Init() {};
    virtual EntityType GetType() = 0;
    virtual void Update(float dt) = 0;
    virtual void Save() {};
    virtual bool Load() { return false; };
    virtual void InputUpdate(float dt, CommandHandler& commands) {};
    virtual void Render(float dt, Camera* camera) {};
    Mat4 GetWorldMatrix();
    WorldPos GetWorldPosition();
    Vec3 GetForwardVector();
    void EntityOnCollisionGeneral(RigidBody& rb, const Vec3& collisionPositionDelta);
    //Quat GetTrueRotation();
};

#define ENTITYBOILERPLATE(name) \
EntityType GetType() override { return EntityType::name; }

struct Player : public Entity
{
    ENTITYBOILERPLATE(Player);
    Inventory m_inventory = {};
    Capsule m_collider = {
    .m_radius = 0.25f,
    .m_height = 1.8f,
    };
    RigidBody m_rigidBody = {};
    uint32 m_inputID = {};
    bool m_hasCamera;
    void ChildCamera(Camera* c);
    void DecoupleCamera();
    void Init() override;
    void InputUpdate(float dt, CommandHandler& commands) override;
    void Update(float dt) override;
    void Render(float dt, Camera* camera) override;
    void Save() override;
    bool Load() override;
};

struct Camera : public Entity
{
    ENTITYBOILERPLATE(Camera);

    //Vec3  m_front  = { 0.0f, 0.0f, -1.0f };
    Vec3  m_up     = { 0.0f, 1.0f, 0.0f };
    Mat4  m_view;
    Mat4  m_perspective;
    Mat4  m_viewProj;
    //float m_yaw   = -90.0f;
    //float m_pitch = 0.0f;
    int32 m_fogDistance = 40;
    int32 m_drawDistance = 10;
    float m_targetSpeed = {};
    Vec3 m_velocity;

    void Update(float dt) override;
    void InputUpdate(float dt, CommandHandler& commands) override;
};


struct Entitys {

    std::vector<Entity*> list;
    template <typename T>
    T* New()
    {
        T* e = new T();
        //assert(constexpr(std::is_same_v<T, Item>))
        //assert(T != Item);
        list.push_back(e);
        return e;
    }
    Entity* GetEntity(EntityID id)
    {
        for (auto e : list)
        {
            if (e->m_ID == id)
                return e;
        }
        return nullptr;
    }
    //template <typename T>
    //T* GetFirst()
    //{
    //    for (auto e : list)
    //    {
    //        if (e->GetType() == T.GetType())
    //            return e;
    //    }
    //    return nullptr;
    //}
    void Add(Entity* entity);
    void Remove(EntityID id);
    void CleanUp();
    void InputUpdate(float deltaTime, CommandHandler& commands);
    void Update(float deltaTime);
    void Render(float deltaTime, Camera* camera);
};


struct Item : public Entity
{
    ENTITYBOILERPLATE(Item);
    void Update(float deltaTime);
    Cube m_collider = {};
    RigidBody m_rigidBody = {};
    BlockType m_type;
    bool m_lootable = true;
    float m_lootableCountDown = 1.0f; //seconds
};

#pragma pack(push, 1)
struct ItemDiskData {
    Transform m_transform;
    std::underlying_type<BlockType>::type m_type;
};
#pragma pack(pop)

struct Items
{
    std::mutex        m_listVectorMutex;
    std::vector<Item> m_items;

    //Item* Add(BlockType blockType, const WorldPos& position, const WorldPos& destination);
    Item* Add(std::vector<EntityID>& itemIDs, BlockType blockType, const WorldPos& position, const WorldPos& destination);
    Item* Get(EntityID ID);
    void Update(float deltaTime);
    void RenderOpaque(float dt, Camera* camera);
    void RenderTransparent(float dt, Camera* camera);
    //void Render(float deltaTime, Camera* camera);
    bool Save(const std::vector<ItemDiskData>& diskData, const ChunkPos& p);
    bool Save(const ChunkPos& p);
    bool SaveAll();
    bool Load(std::vector<EntityID>& itemIDs, const ChunkPos& p);
    void CleanUp();
    //bool Load(const ChunkPos& p, ChunkIndex i);
};

extern Items g_items;
extern Entitys g_entityList;


bool EntityInit();
