#version 420 core
//Color Texture:
layout (binding = 0) uniform sampler2DArray sampler;
//Depth Peel:
layout (binding = 1) uniform sampler2D depthTexture;
//Opaque Depth Pass:
layout (binding = 2) uniform sampler2D opaqueDepthTexture;

in vec3 p_normal;
in vec4 gl_FragCoord;
in vec4 p_color;
in float p_connectedVertices;
in mat4 p_view;

out vec4 color;

uniform uint u_passCount;
uniform vec3 u_ambientLight;
uniform vec3 u_directionalLight_d;
uniform vec3 u_lightColor;
uniform vec3 u_directionalLightMoon_d;
uniform vec3 u_moonColor;

void main()
{
    vec4 depthTexel        = texelFetch(depthTexture,         ivec2(gl_FragCoord.xy), 0);
    vec4 opaqueDepthTexel  = texelFetch(opaqueDepthTexture,   ivec2(gl_FragCoord.xy), 0);
    if (u_passCount == 1)
    {
        if ((gl_FragCoord.z >= opaqueDepthTexel.r))
            discard;
    }
    else if (u_passCount > 1) //I think I can get away with removing this if statement
    {
        if ((gl_FragCoord.z >= opaqueDepthTexel.r) || (gl_FragCoord.z <= (depthTexel.r)))
            discard;
    }

    if (p_color.a == 0)
        discard;

    //
    //AMBIENT Lighting:
    //
    //vec3 ambient = vec3(1.0) * material.ambient;
    vec3 ambient = clamp(u_lightColor + u_moonColor, 0, 1) * u_ambientLight;

    //
    //DIFFUSE Lighting:
    //
    vec3 norm = normalize(p_normal);
    //vec3 constLightDir = vec3(0.2, -.9, 0.1);
    //vec3 lightViewPosition = (p_view * vec4(-constLightDir, 0)).xyz;
    vec3 lightViewPosition = (p_view * vec4(-u_directionalLight_d, 0)).xyz;
    vec3 lightDir = normalize(lightViewPosition);

    vec3 lightViewPositionMoon = (p_view * vec4(-u_directionalLightMoon_d, 0)).xyz;
    vec3 lightDirMoon = normalize(lightViewPositionMoon);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseSun = u_lightColor * diff;

    float diffMoon = max(dot(norm, lightDirMoon), 0.0);
    vec3 diffuseMoon = u_moonColor * diffMoon;
    //vec3 diffuse = vec3(1.0) * (diff * material.diffuse);
    vec3 diffuse = diffuseSun + diffuseMoon;

    //
    //OCCLUSION:
    //
    //float adjustedVertexCount = p_connectedVertices;
    float adjustedVertexCount = min(p_connectedVertices, 2);
    float ambientOcclusion = adjustedVertexCount / 3.0;
    ambientOcclusion = clamp(ambientOcclusion, 0, 1);
    ambientOcclusion = ambientOcclusion * ambientOcclusion * ambientOcclusion;
    //ambientOcclusion = ambientOcclusion * ambientOcclusion;

    //
    //RESULT:
    //
    //vec3 resultColor = (max(ambient + diffuse - ambientOcclusion, 0.01)) * p_color.xyz;
    vec3 resultColor = (max(ambient + diffuse, 0.01)) * p_color.xyz;
    //vec3 resultColor = diffuseSun;
#if 0
    const float e0 = 2.99;
    const float e1 = 1;
    const float e2 = 0;
    if (p_connectedVertices > e0)
        resultColor = vec3( adjustedVertexCount - e0, 0, 0 );
    else if (p_connectedVertices > e1)
        resultColor = vec3( 0, adjustedVertexCount - e1, 0 );
    else if (p_connectedVertices > e2)
        resultColor = vec3( 0, 0, adjustedVertexCount);
#endif

    //if (u_passCount == 0)
    color = vec4(resultColor.xyz, 1);
    //color.xyz = p_normal;
    //color.xyz += (ambientOcclusion / 10000);
}
