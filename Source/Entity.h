#pragma once

#include "Math.h"
#include "Misc.h"
#include "Input.h"
#include "Inventory.h"

enum class MovementType {
    Fly,
    Collision,
};

enum class EntityType {
    Player,
    Camera,
    Item,
};

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
    virtual void InputUpdate(float dt, CommandHandler& commands) {};
    virtual void Update(float dt) = 0;
    virtual void Render(float dt, Camera* camera) {};
    WorldPos RealWorldPos();
    GamePos  RealGamePos();
    ChunkPos RealChunkPos();
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
    MovementType m_movementType = MovementType::Fly;
    uint32 m_inputID = {};
    void Init() override;
    void InputUpdate(float dt, CommandHandler& commands) override;
    void Update(float dt) override;
    void Render(float dt, Camera* camera) override;
};

struct Camera : public Entity
{
    ENTITYBOILERPLATE(Camera);

    Vec3  m_front  = { 0.0f, 0.0f, -1.0f };
    Vec3  m_up     = { 0.0f, 1.0f, 0.0f };
    Mat4  m_view;
    Mat4  m_perspective;
    Mat4  m_viewProj;
    float m_yaw   = -90.0f;
    float m_pitch = 0.0f;
    int32 m_fogDistance = 40;
    int32 m_drawDistance = 10;

    void Update(float dt);
};

struct Item : public Entity 
{
    ENTITYBOILERPLATE(Item);
    BlockType m_type;
    static VertexBuffer m_vertexBuffer;
    void Update(float dt);
    void Render(float dt, Camera* camera) override;
};

struct Entitys {

    std::vector<Entity*> list;
    template <typename T>
    T* New()
    {
        T* e = new T();
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
extern Entitys g_entityList;
