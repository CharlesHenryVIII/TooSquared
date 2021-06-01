#include "Entity.h"

Entitys g_entityList;

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

//PLAYER

void Player::Init()
{
    m_transform.m_p = { 0.0f, 260.0f, 0.0f };
    m_transform.m_pDelta = { 0.0f, 260.0f, 0.0f };
    m_transform.m_vel = {};
    m_transform.m_acceleration = {};
    //how the fuck was I doing this before
    //m_transform.m_terminalVel = 200.0f;
    m_transform.m_terminalVel = { 200.0f, 200.0f, 200.0f };
}

void Player::InputUpdate(float dt, CommandHandler& commands)
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
    if (!camera)
        return;

    if (commands.keyStates[SDLK_0].downThisFrame)
    {
        switch (m_movementType)
        {
        case MovementType::Fly:
            m_movementType = MovementType::Collision;
            break;
        case MovementType::Collision:
            m_movementType = MovementType::Fly;
            break;
        }
    }


    float cameraAcceleration = 0;
    m_transform.m_acceleration = {};
    //std::vector<Triangle> debug_trianglesToDraw;
    float dragFlightCoefficient = 15.0f;
    switch (m_movementType)
    {
    case MovementType::Fly:
        cameraAcceleration = 200.0f; // m/s^2
        m_transform.m_terminalVel.x = m_transform.m_terminalVel.z =
            m_transform.m_terminalVel.y = 10.0f;
        if (commands.keyStates[SDLK_LSHIFT].down)
        {
            cameraAcceleration *= 30;
            m_transform.m_terminalVel.x = m_transform.m_terminalVel.y = m_transform.m_terminalVel.z = 200.0f;
        }
        {
            //Forward
            if (commands.keyStates[SDLK_w].down && commands.keyStates[SDLK_s].down)
                m_transform.m_acceleration;
            else if (commands.keyStates[SDLK_w].down)
                m_transform.m_acceleration += (cameraAcceleration * camera->m_front);
            else if (commands.keyStates[SDLK_s].down)
                m_transform.m_acceleration -= (cameraAcceleration * camera->m_front);
        }
        {
            //Lateral
            if (commands.keyStates[SDLK_a].down && commands.keyStates[SDLK_d].down)
                m_transform.m_acceleration;
            else if (commands.keyStates[SDLK_a].down)
                m_transform.m_acceleration -= (Normalize(Cross(camera->m_front, camera->m_up)) * cameraAcceleration);
            else if (commands.keyStates[SDLK_d].down)
                m_transform.m_acceleration += (Normalize(Cross(camera->m_front, camera->m_up)) * cameraAcceleration);
        }
        if (commands.keyStates[SDLK_LCTRL].down)
            m_transform.m_terminalVel.x = m_transform.m_terminalVel.z = 10.0f;
        if (commands.keyStates[SDLK_SPACE].down)
            m_transform.m_acceleration.y += cameraAcceleration;
        if (commands.keyStates[SDLK_z].down)
            m_transform.m_acceleration.z += cameraAcceleration;
        if (commands.keyStates[SDLK_x].down)
            m_transform.m_acceleration.x += cameraAcceleration;
        m_transform.UpdateDeltaPosition(dt, { dragFlightCoefficient, dragFlightCoefficient, dragFlightCoefficient }, m_collider.m_radius * 2 * m_collider.m_height, 0);
        m_transform.m_p.p += m_transform.m_pDelta.p;
        break;
    case MovementType::Collision:
    {
        cameraAcceleration = 15.0f; // m/s^2
        Vec3 forward = Normalize(Vec3({ camera->m_front.x, 0, camera->m_front.z }));
        m_transform.m_terminalVel.x = m_transform.m_terminalVel.z = 3.0f;
        m_transform.m_terminalVel.y = 50.0f;

        if (commands.keyStates[SDLK_LSHIFT].down)
        {
            cameraAcceleration = 50.0f;
            m_transform.m_terminalVel.x = m_transform.m_terminalVel.z = 8.0f;
        }
        if (commands.keyStates[SDLK_LCTRL].down)
            cameraAcceleration /= 3.0f;
        {
            //Forward
            if (commands.keyStates[SDLK_w].down && commands.keyStates[SDLK_s].down)
                m_transform.m_acceleration;
            else if (commands.keyStates[SDLK_w].down)
                m_transform.m_acceleration += (cameraAcceleration * forward);
            else if (commands.keyStates[SDLK_s].down)
                m_transform.m_acceleration -= (cameraAcceleration * forward);
        }
        {
            //Lateral
            if (commands.keyStates[SDLK_a].down && commands.keyStates[SDLK_d].down)
                m_transform.m_acceleration;
            else if (commands.keyStates[SDLK_a].down)
                m_transform.m_acceleration -= (Normalize(Cross(forward, camera->m_up)) * cameraAcceleration);
            else if (commands.keyStates[SDLK_d].down)
                m_transform.m_acceleration += (Normalize(Cross(forward, camera->m_up)) * cameraAcceleration);
        }
        if (commands.keyStates[SDLK_SPACE].downThisFrame)
        {
            m_transform.m_vel.y += 7.0f;
            m_transform.m_isGrounded = false;
        }
        if (commands.keyStates[SDLK_z].down)
            m_transform.m_acceleration.z += cameraAcceleration;
        if (commands.keyStates[SDLK_x].down)
            m_transform.m_acceleration.x += cameraAcceleration;

        m_transform.UpdateDeltaPosition(dt, { 10.0f, 1.0f, 10.0f }, m_collider.m_radius * 2 * m_collider.m_height);
        //m_collider.UpdateMidTipLocation(m_transform.m_p);
        m_collider.UpdateTailLocation(m_transform.m_p);
        //g_camera.transform.UpdatePosition2(deltaTime, { 10.0f, 1.0f, 10.0f }, playerCollider.m_radius * 2 * playerCollider.m_height);

        m_transform.m_p.p += m_transform.m_pDelta.p;
        Vec3 deltaPosition = {};
        m_transform.m_isGrounded = false;
        if (CapsuleVsWorldBlocks(m_collider, m_transform.m_pDelta.p, deltaPosition, m_collider.m_collidedTriangles))
        {
            //Update Position
            m_transform.m_p.p += deltaPosition;
            m_transform.m_pDelta = {};

            //Zero velocity going into a collision
            Vec3 normalForceDirection = Normalize(deltaPosition);
            Vec3 collisionDirection = normalForceDirection;//-normalForceDirection;
            Vec3 dotProductResults = { DotProduct(Vec3({ m_transform.m_vel.x, 0.0f, 0.0f }), collisionDirection),
                                       DotProduct(Vec3({ 0.0f, m_transform.m_vel.y, 0.0f }), collisionDirection),
                                       DotProduct(Vec3({ 0.0f, 0.0f, m_transform.m_vel.z }), collisionDirection) };

            //TODO: improve to include deflection/angle of collision not just collision in that direction
            if (dotProductResults.x < 0.0f)
            {
                m_transform.m_vel.x = 0.0f;
            }
            if (dotProductResults.y < 0.0f)
            {
                if (m_transform.m_vel.y < 0.0f)
                    m_transform.m_isGrounded = true;
                m_transform.m_vel.y = 0.0f;
            }
            if (dotProductResults.z < 0.0f)
            {
                m_transform.m_vel.z = 0.0f;
            }
        }


        break;
    }
    }
}

void Player::Update(float dt)
{

    //switch (m_movementType)
    //{
    //case MovementType::Fly:
    //    break;
    //case MovementType::Collision:
    //    break;
    //}
}

void Player::Render(float dt, Camera* camera)
{

}


//Camera
void Camera::Update(float dt)
{

}


//Items
void Item::Render(float dt, Camera* camera)
{
    float scale = 0.5f;
    SlowDrawCube(m_transform.m_p, White, scale, camera, Texture::T::Minecraft, m_type);
}

void Item::Update(float dt)
{
    //null
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
        if (e->GetType() == EntityType::Player)
        {
            auto p = reinterpret_cast<Player*>(e);
            if (p->m_inputID == commands.ID)
                p->InputUpdate(dt, commands);
        }
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

