#include "Rendering.h"
#include "Entity.h"


void RenderSkybox(const Camera* playerCamera)
{
    ZoneScopedN("Draw Skybox");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    g_framebuffers->m_post.Bind();

    DepthWrite(false);
    ShaderProgram* sp = g_renderer.programs[+Shader::Sun];
    sp->UseShader();
    sp->UpdateUniformVec3("u_directionalLight_d", 1, g_renderer.sunLight.d.e);
    sp->UpdateUniformVec3("u_sunColor", 1, g_renderer.sunLight.c.e);
    sp->UpdateUniformVec3("u_directionalLightMoon_d", 1, g_renderer.moonLight.d.e);
    sp->UpdateUniformVec3("u_moonColor", 1, g_renderer.moonLight.c.e);
    Mat4 iViewProj;
    Mat4 playerViewProj = playerCamera->m_viewProj;
    gb_mat4_inverse(&iViewProj, &playerViewProj);
    sp->UpdateUniformMat4("u_inverseViewProjection", 1, false, iViewProj.e);
    sp->UpdateUniformVec3("u_cameraPosition", 1, playerCamera->GetWorldPosition().p.e);
    sp->UpdateUniformFloat("u_gameTime", g_gameData.m_currentTime);
    glActiveTexture(GL_TEXTURE1);
    g_renderer.skyBoxNight->Bind();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.skyBoxDay->Bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

bool HeavensInterpolation(float& result, float time, float lo, float hi, float interpolate_lo, float interpolate_hi)
{
    result = 0;
    if (time > lo && time < hi)
    {
        if (time > interpolate_lo && time < interpolate_hi)
        {
            result = 1.0f;
        }
        else
        {
            result = 0.0f;
            if (time <= interpolate_lo)
            {
                //approaching 1.0 as sun comes up and time goes 6.0
                result = fabs((time - lo) / (interpolate_lo - lo));
            }
            else
            {
                //approaching 0.0 as sun goes down and time goes 18.0
                result = fabsf(1 - ((time - interpolate_hi) / (hi - interpolate_hi)));
            }
        }
    }
    else
    {
        result = 0;
        return false;
    }
    return true;
}

void UpdateHeavens(Light_Direction& sun, Light_Direction& moon, const float inGameTime)
{
    //TODO: Refactor this
    ZoneScopedN("Sun/Moon Update");

    float SunRotationRadians = (((inGameTime - 6.0f) / 24) * tau);
    float sunRotationCos = cosf(SunRotationRadians);
    sun.d = Normalize(Vec3({ -sunRotationCos, -sinf(SunRotationRadians),  0.0f }));
    moon.d = -sun.d;
    const Color sunTransitionColor = { 220 / 255.0f,  90 / 255.0f,  40 / 255.0f, 1.0f };
    const Color moonTransitionColor = { 80 / 255.0f,  80 / 255.0f,  90 / 255.0f, 1.0f };

    //TODO: Fix this garbage shit:
    {

        float percentOfSun = 0;
        if (HeavensInterpolation(percentOfSun, inGameTime, 5.9f, 18.1f, 6.1f, 17.9f))
        {
            sun.c.r = Lerp<float>(Lerp<float>(White.r, sunTransitionColor.r, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
            sun.c.g = Lerp<float>(Lerp<float>(White.g, sunTransitionColor.g, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
            sun.c.b = Lerp<float>(Lerp<float>(White.b, sunTransitionColor.b, 1 - percentOfSun), 0.0f, 1 - percentOfSun);
        }
        else
            sun.c = { 0, 0, 0 };

        float percentOfMoon = 0;
        //percentOfMoon = 1 - percentOfSun;
        if (HeavensInterpolation(percentOfMoon, fmodf(inGameTime + 12.0f, 24.0f), 5.9f, 18.1f, 6.1f, 17.9f))
        {
            moon.c.r = Lerp<float>(moonTransitionColor.r, {}, 1 - percentOfMoon);
            moon.c.g = Lerp<float>(moonTransitionColor.g, {}, 1 - percentOfMoon);
            moon.c.b = Lerp<float>(moonTransitionColor.b, {}, 1 - percentOfMoon);
        }
        else
            moon.c = { 0, 0, 0 };

    }
}
//END OF GARBAGE?
