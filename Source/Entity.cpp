#include "Entity.h"
#include "Chunk.h"
#include "Block.h"
#include "tracy-master/Tracy.hpp"


Entitys g_entityList;
Items   g_items;

Vec3 g_forwardVector = { 0.0f, 0.0f, -1.0f };
//Vec4 g_forwardVectorRotation = { 0, 0, -1, 0.0f };
//Vec4 g_forwardVectorPosition = { 0, 0, -1, 1.0f };

bool EntityInit()
{
    bool success = true;
    std::string filename = g_gameData.m_folderPath;
    success &= CreateFolder(filename);
    filename = g_gameData.m_saveFolderPath;
    success &= CreateFolder(filename);
    filename += g_gameData.m_saveFilename;
    success &= CreateFolder(filename);
    filename += "\\Entity_Data";
    success &= CreateFolder(filename);
    return success;
}

void CameraReleaseAndCouple(Player* player, Camera* camera)
{
    assert(player);
    assert(camera);
    if (player && camera)
    {
        if (player->m_hasCamera)
        {
            player->DecoupleCamera();
        }
        else
        {
            player->ChildCamera(camera);
        }
    }
}


Mat4 Entity::GetWorldMatrix()
{
    Mat4 result;
    Mat4 trans;
    Mat4 rot;
    gb_mat4_identity(&result);
    //gb_mat4_from_quat(&rot, m_transform.m_quat);
    gb_mat4_from_quat(&rot, gb_quat_euler_angles(DegToRad(m_transform.m_pitch), DegToRad(m_transform.m_yaw), 0.0f));
    gb_mat4_translate(&trans, m_transform.m_p.p);
    result = trans * rot;

    if (m_parent)
    {
        Entity* e = g_entityList.GetEntity(m_parent);
        if (e)
        {
            //result = e->GetWorldMatrix() * result;
            result = e->GetWorldMatrix() * result;
        }
    }
    return result;
}

WorldPos Entity::GetWorldPosition()
{
    WorldPos result = {};
    result.p = GetWorldMatrix().col[3].xyz;
    return result;
}

Vec3 Entity::GetForwardVector()
{
    return (GetWorldMatrix() * GetVec4(g_forwardVector, 0.0f)).xyz;
}

#if 0
Quat Entity::GetTrueRotation()
{
#if 1
    Mat4 trans = GetWorldMatrix();
    Quat result = {};
    gb_quat_from_mat4(&result, &trans);
#else
    Quat result = m_transform.m_quat;
    if (m_parent)
    {
        Entity* e = g_entityList.GetEntity(m_parent);
        if (e)
        {
            result *= e->GetTrueRotation();
        }
    }
#endif
    return result;
}
#endif

//PLAYER

void Player::Init()
{
    m_transform.m_p = { 0.0f, 260.0f, 0.0f };
    m_rigidBody.m_vel = {};
    m_rigidBody.m_acceleration = {};
    m_rigidBody.m_terminalVel = { 200.0f, 200.0f, 200.0f };
}

void Player::ChildCamera(Camera* c)
{
    m_hasCamera = true;
    c->m_parent = this->m_ID;
    this->m_transform = c->m_transform;
    c->m_transform = {};
    c->m_transform.m_p.p.y = this->m_collider.m_height - this->m_collider.m_radius;
    c->m_transform.m_pitch = this->m_transform.m_pitch;
    this->m_transform.m_pitch = 0;
    this->m_transform.m_p.p -= c->m_transform.m_p.p;
    this->m_children.push_back(c->m_ID);
}

void Player::DecoupleCamera()
{
    Camera* camera = nullptr;
    for (auto c : m_children)
    {
        Entity* e = g_entityList.GetEntity(c);
        if (e)
        {
            if (e->GetType() == EntityType::Camera)
                camera = reinterpret_cast<Camera*>(e);
        }
    }
    assert(camera);
    if (camera)
    {
        camera->m_parent = {};
        float pitch = camera->m_transform.m_pitch;
        camera->m_transform = m_transform;
        camera->m_transform.m_pitch = pitch;
        camera->m_transform.m_p.p.y += this->m_collider.m_height - this->m_collider.m_radius;

        std::erase_if(m_children,
            [camera](EntityID id)
            {
                return id == camera->m_ID;
            });
    }

    m_hasCamera = false;
}

void Player::InputUpdate(float dt, CommandHandler& commands)
{
    Camera* camera = nullptr;
    if (m_hasCamera)
    {
        for (auto c : m_children)
        {
            Entity* e = g_entityList.GetEntity(c);
            if (e)
            {
                if (e->GetType() == EntityType::Camera)
                    camera = reinterpret_cast<Camera*>(e);
            }
        }
        assert(camera);
    }

    if (!camera)
    {
        return;
    }
    else
    {
        m_transform.m_yaw   -= commands.mouse.pDelta.x * commands.mouse.m_sensitivity;
        camera->m_transform.m_pitch -= commands.mouse.pDelta.y * commands.mouse.m_sensitivity;
        camera->m_transform.m_pitch = Clamp<float>(camera->m_transform.m_pitch, -89.5f, 89.5f);
    }


    //Vec3 front = GetTrueRotation() * faceNormals[+Face::Front];
    Vec3 front = GetForwardVector();//(GetWorldMatrix() * g_forwardVectorRotation).xyz;

    float cameraAcceleration = 0;
    m_rigidBody.m_acceleration = {};
    //std::vector<Triangle> debug_trianglesToDraw;

    cameraAcceleration = 15.0f; // m/s^2
    Vec3 forward = Normalize(Vec3({ front.x, 0.0f, front.z }));
    //Vec3 forward = Normalize(Vec3({ camera->m_front.x, 0.0f, camera->m_front.z }));
    m_rigidBody.m_terminalVel.x = m_rigidBody.m_terminalVel.z = 3.0f;
    m_rigidBody.m_terminalVel.y = 50.0f;
    if (commands.keyStates[SDLK_LSHIFT].down)
    {
        cameraAcceleration = 50.0f;
        m_rigidBody.m_terminalVel.x = m_rigidBody.m_terminalVel.z = 8.0f;
    }
    if (commands.keyStates[SDLK_LCTRL].down)
        cameraAcceleration /= 3.0f;
    {
        //Forward
        if (commands.keyStates[SDLK_w].down && commands.keyStates[SDLK_s].down)
            m_rigidBody.m_acceleration;
        else if (commands.keyStates[SDLK_w].down)
            m_rigidBody.m_acceleration += (cameraAcceleration * forward);
        else if (commands.keyStates[SDLK_s].down)
            m_rigidBody.m_acceleration -= (cameraAcceleration * forward);
    }
    {
        //Lateral
        if (commands.keyStates[SDLK_a].down && commands.keyStates[SDLK_d].down)
            m_rigidBody.m_acceleration;
        else if (commands.keyStates[SDLK_a].down)
            m_rigidBody.m_acceleration -= (Normalize(Cross(forward, camera->m_up)) * cameraAcceleration);
        else if (commands.keyStates[SDLK_d].down)
            m_rigidBody.m_acceleration += (Normalize(Cross(forward, camera->m_up)) * cameraAcceleration);
    }
    if (commands.keyStates[SDLK_SPACE].downThisFrame)
    {
        m_rigidBody.m_vel.y += 7.0f;
        m_rigidBody.m_isGrounded = false;
    }
    //if (commands.keyStates[SDLK_z].down)
    //    m_rigidBody.m_acceleration.z += cameraAcceleration;
    //if (commands.keyStates[SDLK_x].down)
    //    m_rigidBody.m_acceleration.x += cameraAcceleration;
}

void Entity::EntityOnCollisionGeneral(RigidBody& rb, const Vec3& collisionPositionDelta)
{
    //Update Position
    m_transform.m_p.p += collisionPositionDelta;

    //Zero velocity going into a collision
    Vec3 normalForceDirection = Normalize(collisionPositionDelta);
    Vec3 collisionDirection = normalForceDirection;//-normalForceDirection;
    Vec3 dotProductResults = { DotProduct(Vec3({ rb.m_vel.x, 0.0f, 0.0f }), collisionDirection),
                               DotProduct(Vec3({ 0.0f, rb.m_vel.y, 0.0f }), collisionDirection),
                               DotProduct(Vec3({ 0.0f, 0.0f, rb.m_vel.z }), collisionDirection) };

    //TODO: improve to include deflection/angle of collision not just collision in that direction
    //consider using Max() instead of if statements like this
    if (dotProductResults.x < 0.0f)
    {
        rb.m_vel.x = 0.0f;
    }
    if (dotProductResults.y < 0.0f)
    {
        if (rb.m_vel.y < 0.0f)
            rb.m_isGrounded = true;
        rb.m_vel.y = 0.0f;
    }
    if (dotProductResults.z < 0.0f)
    {
        rb.m_vel.z = 0.0f;
    }
}

void Player::Update(float dt)
{
    Vec3 kinematicsPositionDelta = m_rigidBody.GetDeltaPosition(dt, { 10.0f, 1.0f, 10.0f });
    m_collider.UpdateTailLocation(m_transform.m_p);

    m_transform.m_p.p += kinematicsPositionDelta;
    Vec3 collisionPositionDelta = {};
    m_rigidBody.m_isGrounded = false;
    //m_rigidBody.VsWorldBlocks();
    if (CapsuleVsWorldBlocks(m_collider, kinematicsPositionDelta, collisionPositionDelta, m_collider.m_collidedTriangles))
    {
        EntityOnCollisionGeneral(m_rigidBody, collisionPositionDelta);
    }


    std::lock_guard<std::mutex> lock(g_items.m_listVectorMutex);
    for (auto& i : g_items.m_items)
    {
        if (i.m_lootable)
        {
            if (Distance(i.m_transform.m_p.p, this->m_collider.m_tail.p) < 1.5f ||
                Distance(i.m_transform.m_p.p, this->m_collider.m_tip.p) < 1.5f)
            {
                i.inUse = (!!m_inventory.Add(i.m_type, 1));
            }
        }
    }
}

void Player::Render(float dt, Camera* camera)
{

}

struct PlayerFileHeader {
    uint32 m_header;
    uint32 m_type;
    uint32 m_version;
};

void Player::Save()
{
    PlayerFileHeader mainHeader = {};
    mainHeader.m_header  = SDL_FOURCC('E', 'N', 'T', 'T');
    mainHeader.m_type    = SDL_FOURCC('P', 'L', 'Y', 'R');
    mainHeader.m_version = 1;

    std::string filename = g_gameData.m_saveFolderPath + g_gameData.m_saveFilename + "\\Player_Data.wad";
    File file(filename, File::Mode::Write, true);
    if (file.m_handleIsValid)
    {
        bool success = true;
        success &= file.Write(&mainHeader,  sizeof(PlayerFileHeader));
        success &= file.Write(&m_transform, sizeof(m_transform));
        success &= file.Write(&m_inventory, sizeof(Inventory));
        success &= file.Write(&m_rigidBody, sizeof(RigidBody));
    }
}

bool Player::Load()
{
    std::string filename = g_gameData.m_saveFolderPath + g_gameData.m_saveFilename + "\\Player_Data.wad";
    File file(filename, File::Mode::Read, false);
    if (file.m_handleIsValid)
    {
        file.GetData();
        if (file.m_binaryDataIsValid)
        {
            uint32 header = SDL_FOURCC('E', 'N', 'T', 'T');
            uint32 type = SDL_FOURCC('P', 'L', 'Y', 'R');
            uint32 version = 1;

            PlayerFileHeader* mainHeader = (PlayerFileHeader*)file.m_dataBinary.data();
            if (mainHeader->m_header == header && mainHeader->m_type == type && mainHeader->m_version == version)
            {
                Transform* tran = (Transform*)(mainHeader + 1);
                Inventory* inv = (Inventory*)(tran + 1);
                RigidBody* rb = (RigidBody*)(inv + 1);

                m_transform = *tran;
                m_inventory = *inv;
                m_rigidBody = *rb;

                return true;
            }
        }
    }
    return false;
}



//Camera
void Camera::Update(float dt)
{
    //Entity* e = g_entityList.GetEntity(m_parent);
    //if (e == nullptr)
    //{
    //}
}

void Camera::InputUpdate(float dt, CommandHandler& commands)
{
    Entity* e = g_entityList.GetEntity(m_parent);
    if (e == nullptr)
    {
        m_transform.m_yaw -= commands.mouse.pDelta.x * commands.mouse.m_sensitivity;
        m_transform.m_pitch -= commands.mouse.pDelta.y * commands.mouse.m_sensitivity;
        m_transform.m_pitch = Clamp<float>(m_transform.m_pitch, -89.5f, 89.5f);

        Vec3 request = {};
        if (commands.keyStates[SDLK_w].down)
            request += gb_vec3(0, 0, -1);
        if (commands.keyStates[SDLK_s].down)
            request += gb_vec3(0, 0, 1);
        if (commands.keyStates[SDLK_a].down)
            request += gb_vec3(-1, 0, 0);
        if (commands.keyStates[SDLK_d].down)
            request += gb_vec3(1, 0, 0);
        if (commands.keyStates[SDLK_SPACE].down)
            request += gb_vec3(0, 1, 0);
        if (commands.keyStates[SDLK_LCTRL].down)
            request += gb_vec3(0, -1, 0);

        m_targetSpeed = 10.0f;
        if (commands.keyStates[SDLK_LSHIFT].down)
            m_targetSpeed = 200.0f;

        Vec3 front = (GetWorldMatrix() * GetVec4(request, 0 )).xyz;
        gb_vec3_norm0(&front, front);
        Vec3 targetVelocity = front * m_targetSpeed;

        m_velocity = Converge(m_velocity, targetVelocity, 16.0f, dt);
        m_transform.m_p.p += m_velocity * dt;
        //m_transform.UpdateCameraPosition(dt, targetVelocity);
    }
}


//Item
void Item::Update(float dt)
{
#if 1

    Vec3 kinematicsPositionDelta = m_rigidBody.GetDeltaPosition(dt, { 3.0f, 1.0f, 3.0f });
    m_collider.m_center = m_transform.m_p;
    m_transform.m_p.p += kinematicsPositionDelta;
    Vec3 collisionPositionDelta = {};
    m_rigidBody.m_isGrounded = false;
    if (CubeVsWorldBlocks(m_collider, kinematicsPositionDelta, collisionPositionDelta, m_collider.m_collidedTriangles))
    {
        EntityOnCollisionGeneral(m_rigidBody, collisionPositionDelta);
    }

    //Mat4 result;
    //Mat4 translation;
    //Mat4 rotation;
    //gb_mat4_identity(&result);
    //gb_mat4_from_quat(&rotation, gb_quat_euler_angles(DegToRad(i.m_transform.m_pitch), DegToRad(i.m_transform.m_yaw), 0.0f));
    //gb_mat4_rotate(&rotation, { 0,1,0 }, (totalTime * 3.0f) / (2 * 3.14f));
    //gb_mat4_translate(&translation, { i.m_transform.m_p.p.x, i.m_transform.m_p.p.y - (scale / 2.0f), i.m_transform.m_p.p.z });
    //gb_mat4_translate(&translation, m_transform.m_p.p); // based on m_transform being the center
    //result = translation * rotation;
    //result = translation;
    const float scale = 0.5f;
    //AddCubeToRender(m_transform.m_p, White, scale);
#if DEBUG_BLOCKRENDER == 1
    WorldPos positionWithScale = m_transform.m_p;
    positionWithScale.p.y += ((scale - 1) / 2);
    AddBlockToRender(positionWithScale, scale, m_type);
#elif DEBUG_BLOCKRENDER == 2
    AddBlockToRender(m_transform.m_p, scale, m_type);
#endif
#else

    //Old Simple Movement Code
    GamePos blockInsideP = ToGame(m_transform.m_p.p);
    GamePos blockBelowP = blockInsideP;
    blockBelowP.p.y = Max(0, blockBelowP.p.y - 1);
    BlockType blockInsideType;
    BlockType blockBelowType;
    bool moved = false;
    while (g_chunks->GetBlock(blockInsideType, blockInsideP) && blockInsideType != BlockType::Empty)
    {
        m_transform.m_p.p.y += 1.0f;
        blockInsideP.p.y += 1;
        moved = true;
    }
    if (!moved)
    {
        while (g_chunks->GetBlock(blockBelowType, blockBelowP) && blockBelowType == BlockType::Empty)
        {
            m_transform.m_p.p.y -= 1.0f;
            blockBelowP.p.y -= 1;
        }
    }

    //Apply angular velocity
    float angularVelocity = tau / 5; // rads per second
    //e.m_transform.m_quat *= gb_quat_euler_angles(0.0f, yaw, 0.0f);
    m_transform.m_yaw += RadToDeg(dt * angularVelocity);
#endif

    if (!m_lootable && (m_lootableCountDown > 0.0f))
    {
        m_lootableCountDown -= dt;
        m_lootable = m_lootableCountDown <= 0.0f;
    }
}



//Items
Item* Items::Add(std::vector<EntityID>& itemIDs, BlockType blockType, const WorldPos& position, const WorldPos& destination)
{
    Item newItem;
    assert(blockType != BlockType::Empty);
    newItem.m_transform.m_p.p = position.p;
    if (newItem.m_transform.m_p.p.y < 0)
        newItem.m_transform.m_p.p.y = ToWorld(GamePos({ 0, CHUNK_Y, 0})).p.y;
    newItem.m_type = blockType;
    newItem.m_collider.m_length = 0.5f;
    newItem.m_lootable = false;
    newItem.m_lootableCountDown = 1.0f; //seconds

    Vec3 velocity = destination.p - position.p;
    if (!(velocity == Vec3({})))
    {
        velocity = NormalizeZero(velocity);
        velocity.y = 0.5f;
        velocity = NormalizeZero(velocity);
        velocity *= 8;
    }
    itemIDs.push_back(newItem.m_ID);

    newItem.m_rigidBody.m_vel = velocity;
    newItem.m_rigidBody.m_terminalVel = { 100.0f, 100.0f, 100.0f };
    std::lock_guard<std::mutex> lock(m_listVectorMutex);
    if (g_blocks[+blockType].m_translucent)
    {
        m_items.push_back(newItem);
        return &m_items[m_items.size() - 1];
    }
    else
    {
        m_items.insert(m_items.begin(), newItem);
        return &m_items[0];
    }
}

//Must Lock Before?
Item* Items::Get(EntityID ID)
{
    assert(OnMainThread());
    for (int32 i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].m_ID == ID)
            return &m_items[i];
    }
    return nullptr;
}

void Items::Update(float dt)
{
    std::lock_guard<std::mutex> lock(m_listVectorMutex);
    for (auto& e : m_items)
    {
        if (e.inUse)
            e.Update(dt);
    }
}


#pragma pack(push, 1)
struct EntityDiskFileHeader {
    uint32 m_magic_header;
    uint32 m_magic_type;
    uint32 version;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ItemDiskHeader {
    uint32 m_magic_header;
    uint32 m_magic_type;
};
#pragma pack(pop)

bool Items::SaveAll()
{
    EntityDiskFileHeader mainHeader;
    mainHeader.m_magic_header = SDL_FOURCC('E', 'N', 'T', 'Y');
    mainHeader.m_magic_type   = SDL_FOURCC('D', 'A', 'T', 'A');
    mainHeader.version = 1;

    ItemDiskHeader itemHeader;
    itemHeader.m_magic_header = SDL_FOURCC('I', 'T', 'E', 'M');
    itemHeader.m_magic_type   = SDL_FOURCC('D', 'A', 'T', 'A');

    ItemDiskData itemData = {};

    bool succeeded = true;
    std::lock_guard<std::mutex> lock(m_listVectorMutex);
    while (m_items.size())
    {
        succeeded &= Save(ToChunk(m_items[0].m_transform.m_p));
    }
    return succeeded;
}

bool Items::Save(const std::vector<ItemDiskData>& diskData, const ChunkPos& p)
{
    EntityDiskFileHeader mainHeader;
    mainHeader.m_magic_header = SDL_FOURCC('E', 'N', 'T', 'Y');
    mainHeader.m_magic_type = SDL_FOURCC('D', 'A', 'T', 'A');
    mainHeader.version = 1;

    ItemDiskHeader itemHeader;
    itemHeader.m_magic_header = SDL_FOURCC('I', 'T', 'E', 'M');
    itemHeader.m_magic_type = SDL_FOURCC('D', 'A', 'T', 'A');

    bool succeeded = true;
    std::string entityFilePath = GetEntitySaveFilePathFromChunkPos(p);
    File file(entityFilePath, File::Mode::Write, true);
    if (diskData.size())
    {
        if (file.m_handleIsValid)
        {
            {
                succeeded &= file.Write(&mainHeader, sizeof(mainHeader));
                succeeded &= file.Write(&itemHeader, sizeof(itemHeader));
                succeeded &= file.Write(diskData.data(), diskData.size() * sizeof(ItemDiskData));
            }
        }
    }
    else
    {
        if (file.m_handleIsValid)
            succeeded &= file.Delete();
        else
            succeeded = false;
    }
    return succeeded;
}

bool Items::Save(const ChunkPos& p)
{
    ItemDiskData itemData = {};

    std::vector<ItemDiskData> itemDiskData;
    itemDiskData.reserve(200);

    std::lock_guard<std::mutex> lock(m_listVectorMutex);
    for (auto& i : m_items)
    {
        ChunkPos checkChunkPos = ToChunk(i.m_transform.m_p);
        if (checkChunkPos.p == p.p)
        {
            itemData.m_transform = i.m_transform;
            itemData.m_type = +i.m_type;
            itemDiskData.push_back(itemData);
            i.inUse = false;
        }
    }
    return Save(itemDiskData, p);
}


bool Items::Load(std::vector<EntityID>& itemIDs, const ChunkPos& p)
//bool Items::Load(const ChunkPos& p, ChunkIndex i)
{
    EntityDiskFileHeader mainHeaderRef;
    mainHeaderRef.m_magic_header = SDL_FOURCC('E', 'N', 'T', 'Y');
    mainHeaderRef.m_magic_type = SDL_FOURCC('D', 'A', 'T', 'A');
    mainHeaderRef.version = 1;
    ItemDiskHeader itemHeaderRef;
    itemHeaderRef.m_magic_header = SDL_FOURCC('I', 'T', 'E', 'M');
    itemHeaderRef.m_magic_type = SDL_FOURCC('D', 'A', 'T', 'A');
    ItemDiskData itemData = {};

    std::string entityFilePath = GetEntitySaveFilePathFromChunkPos(p);
    File file(entityFilePath, File::Mode::Read, false);
    bool success = true;
    if (file.m_handleIsValid)
    {
        file.GetData();
        if (file.m_binaryDataIsValid)
        {
            //TODO: Simplify or put this into a function maybe put it into a parent struct since this type of code seems to be common between saving
            EntityDiskFileHeader* mainHeader = (EntityDiskFileHeader*)file.m_dataBinary.data();
            if (mainHeader->m_magic_header == mainHeaderRef.m_magic_header && mainHeader->m_magic_type == mainHeaderRef.m_magic_type && mainHeader->version == mainHeaderRef.version)
            {
                ItemDiskHeader* itemHeader = (ItemDiskHeader*)(mainHeader + 1);
                if (itemHeader->m_magic_header == itemHeaderRef.m_magic_header && itemHeader->m_magic_type == itemHeaderRef.m_magic_type)
                {
                    size_t count = (file.m_dataBinary.size() - sizeof(EntityDiskFileHeader) - sizeof(ItemDiskHeader)) / sizeof(ItemDiskData);
                    ItemDiskData* itemDataStart = (ItemDiskData*)(itemHeader + 1);
                    for (size_t i = 0; i < count; i++)
                    {
                        Item* item = Add(itemIDs, (BlockType)itemDataStart[i].m_type, itemDataStart[i].m_transform.m_p, itemDataStart[i].m_transform.m_p);
                    }
                }
            }
        }
    }
    else
        success = false;
    return success;
}

void Items::CleanUp()
{
    std::lock_guard<std::mutex> lock(m_listVectorMutex);
    std::erase_if(m_items,
        [](const Entity& e)
        {
            return (!(e.inUse));
        });
}

//Entitys
void Entitys::Add(Entity* entity)
{
    list.push_back(entity);
}

void Entitys::Update(float dt)
{
    for (auto e : list)
    {
        e->Update(dt);
    }
}

void Entitys::Render(float dt, Camera* camera)
{
    for (auto e : list)
    {
        e->Render(dt, camera);
    }
}

void Entitys::Remove(EntityID ID)
{
    for (auto e : list)
    {
        if (e->m_ID == ID)
            e->inUse = false;
    }
}

void Entitys::InputUpdate(float dt,CommandHandler& commands)
{
    ZoneScopedN("Entity Input Update");
    for (auto e : list)
    {
        e->InputUpdate(dt, commands);
    }
}

void Entitys::CleanUp()
{
    std::erase_if(list,
        [](Entity* e)
        {
            if (!e->inUse)
            {
                delete e;
                return true;
            }
            return false;
        });
}
