#version 330 core
uniform samplerCube sampler;

in vec2 p_clipspace;
out vec4 color;

uniform vec3 u_directionalLight_d;
uniform mat4 u_inverseViewProjection;
uniform vec3 u_cameraPosition;

void main()
{
    vec4 unprojected = u_inverseViewProjection * vec4(p_clipspace, 0.0, 1.0);
    unprojected.xyz /= unprojected.w;

    vec3 V = normalize(unprojected.xyz - u_cameraPosition); // view direction
    vec3 skybox = texture(sampler, V).xyz;

    float sunAmount = distance(V, -u_directionalLight_d); // sun falloff descreasing from mid point
    vec3 sunColor = vec3(1);
    vec3 sun = smoothstep(0.03, 0.026, sunAmount) * sunColor * 50.0; // sun disc
    sun += 0.5 * (1 - sunAmount);
    //color = vec4(sun.xyz, 1) + skybox;
    vec3 skyColor = skybox + clamp(sun, 0, 1);
    vec3 fogColor = vec3(0.5);

    color.xyz = V;

    float dotResult = max(dot(V, vec3(0, 1, 0)), 0);
    color.xyz = vec3(dotResult);
    color.xyz = mix(fogColor, skyColor, smoothstep(0.02, 0.1, dotResult));
    color.a = 1;

    //color = vec4(1 - sunAmount);
    //color.a = 1;
}
