#version 420 core
#extension GL_EXT_texture_array : enable
#define DIRECTIONALLIGHT 1
//Texture Map:
layout (binding = 0) uniform sampler2DArray sampler;
//Depth Peel:
layout (binding = 1) uniform sampler2D depthTexture;
//Opaque Depth Pass:
layout (binding = 2) uniform sampler2D opaqueDepthTexture;

in vec2 p_uv;
flat in float p_depth;
in vec3 p_normal;
in vec3 p_pixelP;
in mat4 p_view;
in float p_connectedVertices;
//in vec4 gl_FragCoord;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

uniform ivec2 u_screenSize;
uniform uint u_passCount;
#if DIRECTIONALLIGHT == 1
uniform vec3 u_directionalLight_d;
uniform vec3 u_lightColor;
uniform vec3 u_directionalLightMoon_d;
uniform vec3 u_moonColor;
#else
uniform vec3 u_lightColor;
uniform vec3 u_lightP;
#endif
uniform vec3 u_cameraP;
uniform float u_reflect;
uniform Material material;

out vec4 color;


void main()
{


#if 1
    vec4 depthTexelFetch        = texelFetch(depthTexture,         ivec2(gl_FragCoord.xy), 0);
    vec4 opaqueDepthTexelFetch  = texelFetch(opaqueDepthTexture,   ivec2(gl_FragCoord.xy), 0);
#else
    vec4 depthTexelFetch = texture(depthTexture, gl_FragCoord.xy);
#endif

//Discard fragment if it is not going to peel the current layer 
//or if it would be occluded by opaque geometry
#if 1

    if (u_passCount == 1)
    {
        if ((gl_FragCoord.z >= opaqueDepthTexelFetch.r))
        {
            discard;
        }
    }
    else if (u_passCount > 1) //I think I can get away with removing this if statement
    {
        if ((gl_FragCoord.z >= opaqueDepthTexelFetch.r) || (gl_FragCoord.z <= (depthTexelFetch.r)))
        {
            discard;
        }
    }
#endif

    vec4 pixel = texture2DArray(sampler, vec3(p_uv, p_depth));

#if 1
    if (pixel.a == 0.0)
        discard;

#endif

    //
    //AMBIENT Lighting:
    //
    //vec3 ambient = vec3(1.0) * material.ambient;
    vec3 ambient = clamp(u_lightColor + u_moonColor, 0, 1) * material.ambient;


    //
    //DIFFUSE Lighting:
    //
    vec3 norm = normalize(p_normal);
#if DIRECTIONALLIGHT == 1
    //vec3 constLightDir = vec3(0.2, -.9, 0.1);
    //vec3 lightViewPosition = (p_view * vec4(-constLightDir, 0)).xyz;
    vec3 lightViewPosition = (p_view * vec4(-u_directionalLight_d, 0)).xyz;
    vec3 lightDir = normalize(lightViewPosition);

    vec3 lightViewPositionMoon = (p_view * vec4(-u_directionalLightMoon_d, 0)).xyz;
    vec3 lightDirMoon = normalize(lightViewPositionMoon);
#else
    vec3 lightViewPosition = (p_view * vec4(u_lightP, 1)).xyz;
    vec3 lightDir = normalize(lightViewPosition - p_pixelP);
#endif
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseSun = u_lightColor * (diff * material.diffuse);

    float diffMoon = max(dot(norm, lightDirMoon), 0.0);
    vec3 diffuseMoon = u_moonColor * (diffMoon * material.diffuse);
    //vec3 diffuse = vec3(1.0) * (diff * material.diffuse);
    vec3 diffuse = diffuseSun + diffuseMoon;
    

    //
    //SPECULAR Lighting:
    //
#if DIRECTIONALLIGHT == 1
    vec3 specular = vec3(0);
#else
    vec3 viewDir = normalize(vec3(0) - p_pixelP);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //vec3 specular = u_lightColor * (spec * material.specular);
    vec3 specular = vec3(1.0) * (spec * material.specular);
#endif
    

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
    vec3 result = (max(ambient + diffuse + specular - ambientOcclusion, 0.01)) * pixel.xyz;
    //ambient = vec3(1);
    //result = (max(ambient - ambientOcclusion, 0)) * pixel.xyz;


    //
    //DEBUG:
    //
#if 0
    if (ambientOcclusion > 0)
    {
        result = vec3(1) - vec3(ambientOcclusion, ambientOcclusion, ambientOcclusion);
    }
#endif
#if 0
    const float e0 = 2.99;
    const float e1 = 1;
    const float e2 = 0;
    if (p_connectedVertices > e0)
        result = vec3( adjustedVertexCount - e0, 0, 0 );
    else if (p_connectedVertices > e1)
        result = vec3( 0, adjustedVertexCount - e1, 0 );
    else if (p_connectedVertices > e2)
        result = vec3( 0, 0, adjustedVertexCount);
#endif
    //result = vec3(p_connectedVertices / 2);

    if (u_passCount == 0)
        color = vec4(result, 1.0);
    else
        color = vec4(result, pixel.a);

    //color.r = float(depthTextureSize.x - 1023);
    //color.r = float(depthTextureSize.y - 575);

    //color = vec4(pixelPositionOnTexture.x / depthTextureSize.x, pixelPositionOnTexture.y / depthTextureSize.y, 0, 1.0);
    //color = vec4(float(depthTextureSize.x) * xRatio, float(depthTextureSize.y) * yRatio, 0, 1.0);
    //color = vec4(xRatio, yRatio, 0, 1.0);
    //color.xyz = vec3(depthTexelFetch.x);

    //color.r = depthTexelFetch.r - 0.9;
    //color = pixel;
    //float(depthTextureSize.x) / 1024;
    //color.r = (depthTexelFetch.r - 0.999) / (1 - 0.999);
    //color.g = 1 - depthTexelFetch.r;
    //color.b = 0;
    //color.a = 1;
    //float(u_screenSize.x);//gl_FragCoord.x;
    //if (u_passCount != 0)
    //{
    //    color.xyz = vec3(0);
    //    color.r = depthTexelFetch.z;
    //}

    //color.xyz = p_normal;
    //color.a = 1;
    //color.xyz = vec3(p_uv * 16, 0);
}
