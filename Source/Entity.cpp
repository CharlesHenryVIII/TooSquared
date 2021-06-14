#include "Entity.h"

Entitys g_entityList;
Items   g_items;

Vec4 g_forwardVectorRotation = { 0, 0, -1, 0.0f };
Vec4 g_forwardVectorPosition = { 0, 0, -1, 1.0f };

#if 0
WorldPos Entity::RealWorldPos()
{
    WorldPos result = m_transform.m_p;
    if (m_parent)
    {
        Entity* e = g_entityList.GetEntity(m_parent);
        if (e)
            result.p += e->RealWorldPos().p;
    }
    return result;
}

GamePos  Entity::RealGamePos()
{
    return ToGame(RealWorldPos());
}

ChunkPos Entity::RealChunkPos()
{
    return ToChunk(RealWorldPos());
}
#endif

Mat4 Entity::GetTranslationMatrix()
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
            //result = e->GetTranslationMatrix() * result;
            result = e->GetTranslationMatrix() * result;
        }
    }
    return result;
}

WorldPos Entity::GetTruePosition()
{
#if 1
    WorldPos result = {};
    result.p = GetTranslationMatrix().col[3].xyz;
#else
    Vec4 a = { 1,1,1,1 };//{ m_transform.m_p.p.x, m_transform.m_p.p.y, m_transform.m_p.p.z, 1.0f };
    a = GetTranslationMatrix() * a;
    WorldPos result;
    result.p = a.xyz;
#endif
    return result;
}

#if 0
Quat Entity::GetTrueRotation()
{
#if 1
    Mat4 trans = GetTranslationMatrix();
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
    Vec3 front = (GetTranslationMatrix() * g_forwardVectorRotation).xyz;

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
    if (commands.keyStates[SDLK_z].down)
        m_rigidBody.m_acceleration.z += cameraAcceleration;
    if (commands.keyStates[SDLK_x].down)
        m_rigidBody.m_acceleration.x += cameraAcceleration;
}

void Player::Update(float dt)
{
    Vec3 kinematicsPositionDelta = m_rigidBody.GetDeltaPosition(dt, { 10.0f, 1.0f, 10.0f }, m_collider.m_radius * 2 * m_collider.m_height);
    //m_collider.UpdateMidTipLocation(m_transform.m_p);
    m_collider.UpdateTailLocation(m_transform.m_p);
    //g_camera.transform.UpdatePosition2(deltaTime, { 10.0f, 1.0f, 10.0f }, playerCollider.m_radius * 2 * playerCollider.m_height);

    m_transform.m_p.p += kinematicsPositionDelta;
    Vec3 collisionPositionDelta = {};
    m_rigidBody.m_isGrounded = false;
    if (CapsuleVsWorldBlocks(m_collider, kinematicsPositionDelta, collisionPositionDelta, m_collider.m_collidedTriangles))
    {
        //Update Position
        m_transform.m_p.p += collisionPositionDelta;

        //Zero velocity going into a collision
        Vec3 normalForceDirection = Normalize(collisionPositionDelta);
        Vec3 collisionDirection = normalForceDirection;//-normalForceDirection;
        Vec3 dotProductResults = { DotProduct(Vec3({ m_rigidBody.m_vel.x, 0.0f, 0.0f }), collisionDirection),
                                   DotProduct(Vec3({ 0.0f, m_rigidBody.m_vel.y, 0.0f }), collisionDirection),
                                   DotProduct(Vec3({ 0.0f, 0.0f, m_rigidBody.m_vel.z }), collisionDirection) };

        //TODO: improve to include deflection/angle of collision not just collision in that direction
        if (dotProductResults.x < 0.0f)
        {
            m_rigidBody.m_vel.x = 0.0f;
        }
        if (dotProductResults.y < 0.0f)
        {
            if (m_rigidBody.m_vel.y < 0.0f)
                m_rigidBody.m_isGrounded = true;
            m_rigidBody.m_vel.y = 0.0f;
        }
        if (dotProductResults.z < 0.0f)
        {
            m_rigidBody.m_vel.z = 0.0f;
        }
    }


    for (auto& i : g_items.m_items)
    {
        if (Distance(i.m_transform.m_p.p, this->m_collider.m_tail.p) < 1.5f ||
            Distance(i.m_transform.m_p.p, this->m_collider.m_tip.p)  < 1.5f)
        {
            m_inventory.Add(i.m_type, 1);
            i.m_lootable = false;
        }
    }
}

void Player::Render(float dt, Camera* camera)
{
    
}


//Camera
void Camera::Update(float dt)
{
    Entity* e = g_entityList.GetEntity(m_parent);
    if (e == nullptr)
    {
        //const float dragFlightCoefficient = 15.0f;
        //const float fakeAreaToGetThisToWork = 0.25f * 2 * 1.8f;//m_collider.m_radius * 2 * m_collider.m_height;

        //m_transform.UpdateDeltaPosition(dt, { dragFlightCoefficient, dragFlightCoefficient, dragFlightCoefficient }, fakeAreaToGetThisToWork, 0);
        //m_transform.m_p.p += m_transform.m_pDelta.p;
        //m_transform.UpdateCameraPosition(dt, );
    }
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

        Vec3 front = (GetTranslationMatrix() * Vec4 { request.x, request.y, request.z, 0 }).xyz;
        gb_vec3_norm0(&front, front);
        Vec3 targetVelocity = front * m_targetSpeed;

        m_velocity = Converge(m_velocity, targetVelocity, 16.0f, dt);
        m_transform.m_p.p += m_velocity * dt;
        //m_transform.UpdateCameraPosition(dt, targetVelocity);
    }
}


//Items
Item* Items::Add(BlockType blockType, WorldPos position)
{
    Item newItem;
    newItem.m_transform.m_p.p = position.p;
    newItem.m_type = blockType;
    m_items.push_back(newItem);
    return &m_items[m_items.size() - 1];
}

void Items::Update(float dt)
{
    std::erase_if(m_items,
        [](Item& i)
        {
            return (!i.m_lootable);
        });

    for (auto& e : m_items)
    {
        GamePos blockInsideP = ToGame(e.m_transform.m_p.p);
        GamePos blockBelowP = blockInsideP;
        blockBelowP.p.y = Max(0, blockBelowP.p.y - 1);
        BlockType blockInsideType;
        BlockType blockBelowType;
        bool moved = false;
        while (g_chunks->GetBlock(blockInsideType, blockInsideP) && blockInsideType != BlockType::Empty)
        {
            e.m_transform.m_p.p.y += 1.0f;
            blockInsideP.p.y += 1;
            moved = true;
        }
        if (!moved)
        {
            while (g_chunks->GetBlock(blockBelowType, blockBelowP) && blockBelowType == BlockType::Empty)
            {
                e.m_transform.m_p.p.y -= 1.0f;
                blockBelowP.p.y -= 1;
            }
        }
    }

    //Apply angular velocity
    float angularVelocity = tau / 5; // rads per second
    for (auto& e : m_items)
    {
        float yaw = dt * angularVelocity;
        //e.m_transform.m_quat *= gb_quat_euler_angles(0.0f, yaw, 0.0f);

        e.m_transform.m_yaw += dt * angularVelocity;
    }
}

void Items::Render(float dt, Camera* camera)
{
    float scale = 0.5f;
    for (auto i : m_items)
    {
        Mat4 result;
        Mat4 translation;
        Mat4 rotation;
        gb_mat4_identity(&result);
        gb_mat4_from_quat(&rotation, gb_quat_euler_angles(DegToRad(i.m_transform.m_pitch), DegToRad(i.m_transform.m_yaw), 0.0f));
        //gb_mat4_rotate(&rotation, { 0,1,0 }, (totalTime * 3.0f) / (2 * 3.14f));
        gb_mat4_translate(&translation, i.m_transform.m_p.p);
        result = translation * rotation;
        DrawBlock(result, White, scale, camera, Texture::T::Minecraft, i.m_type);
    }
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

