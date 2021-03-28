#version 420 core
layout(binding = 0) uniform samplerCube s_skyboxDay;
layout(binding = 1) uniform samplerCube s_skyboxNight;

in vec2 p_clipspace;
out vec4 color;

uniform vec3 u_directionalLight_d;
uniform vec3 u_sunColor;
uniform vec3 u_directionalLightMoon_d;
uniform vec3 u_moonColor;
uniform float u_gameTime;
uniform mat4 u_inverseViewProjection;
uniform vec3 u_cameraPosition;

void main()
{
    vec4 unprojected = u_inverseViewProjection * vec4(p_clipspace, 0.0, 1.0);
    unprojected.xyz /= unprojected.w;

//_____SKYBOX_____
    vec3 V = normalize(unprojected.xyz - u_cameraPosition); // view direction
    vec3 skyboxDay = texture(s_skyboxDay, V).xyz;
    vec3 skyboxNight = texture(s_skyboxNight, V).xyz;
    //float skyboxMixAmount = smoothstep(0.0, 0.4, mod(u_gameTime, 12.0));
    //vec3 skybox = mix(skyboxDay, skyboxNight, skyboxMixAmount);
    //vec3 skybox = mix(skyboxDay, skyboxNight, 1.0);
    vec3 skybox;
    float threshold = 0.5;
    if (u_gameTime > 6 + threshold && u_gameTime < 18 - threshold)
        skybox = skyboxDay;
    else if (u_gameTime > 18 + threshold || u_gameTime < 6 - threshold)
        skybox = skyboxNight;
    else if (u_gameTime >= 6 - threshold && u_gameTime <= 6 + threshold)
        skybox = mix(skyboxNight, skyboxDay, (u_gameTime - (6 - threshold)) / (threshold * 2));
    else
        skybox = mix(skyboxDay, skyboxNight, (u_gameTime - (18 - threshold)) / (threshold * 2));
   // vec3 skyboxDayM   = skyboxDay   * smoothstep(0, 12, (u_gameTime - 6));
   // vec3 skyboxNightM = skyboxNight * smoothstep(12, 24, (u_gameTime - 6));
   // vec3 skybox = skyboxDayM + skyboxNightM;

//___SUN AMOUNT___
    float sunAmount = distance(V, -u_directionalLight_d); // sun falloff descreasing from mid point
    vec3 sunColor = u_sunColor;
    vec3 sun = smoothstep(0.03, 0.026, sunAmount) * sunColor * 5.0; // sun disc
    sun += 0.5 * (1 - sunAmount);
    sun = clamp(sun, 0, 1);
    //color = clamp(vec4(sun.xyz + skybox.xyz, 1), 0, 1);

//___MUN AMOUNT___
    float moonAmount = distance(V, -u_directionalLightMoon_d); // moon falloff descreasing from mid point
    vec3 moonColor = u_moonColor;
    vec3 moon = smoothstep(0.03, 0.026, moonAmount) * moonColor.xyz * 4.0; // moon disc
    moon += 0.1 * (1 - moonAmount);
    moon = clamp(moon, 0, 1);

//___SKY COLOR____
    //color = vec4(sun.xyz + skybox, 1);
    vec3 skyColor = clamp(skybox.xyz + sun + moon.xyz, 0, 1);
    vec3 fogColor = vec3(0.5);

//    color.xyz = V;

//    float dotResult = max(dot(V, vec3(0, 1, 0)), 0);
//    color.xyz = vec3(dotResult);
//    color.xyz = mix(fogColor, skyColor, smoothstep(0.02, 0.1, dotResult));
    color.a = 1;
    color.xyz = skyColor;

    //color = vec4(1 - sunAmount);
    //color.a = 1;
}
