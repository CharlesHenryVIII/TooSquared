#version 330 core
#extension GL_EXT_texture_array : enable
#define DIRECTIONALLIGHT 1
uniform sampler2DArray sampler;

in vec2 p_uv;
flat in float p_depth;
in vec3 p_normal;
in vec3 p_pixelP;
in mat4 p_view;
in float p_connectedVertices;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

#if DIRECTIONALLIGHT == 1
uniform vec3 u_directionalLight_d;
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
    vec4 pixel = texture2DArray(sampler, vec3(p_uv, p_depth));

    //Ambient Lighting:
    vec3 ambient = vec3(1.0) * material.ambient;
    //vec3 ambient = u_lightColor * material.ambient;

    //Diffuse Lighting:
    vec3 norm = normalize(p_normal);
#if DIRECTIONALLIGHT == 1
    vec3 lightViewPosition = (p_view * vec4(-u_directionalLight_d, 0)).xyz;
    //vec3 lightViewPosition = (p_view * vec4(-(vec3(1, -1, 0)), 0)).xyz;
    vec3 lightDir = normalize(lightViewPosition);
#else
    vec3 lightViewPosition = (p_view * vec4(u_lightP, 1)).xyz;
    vec3 lightDir = normalize(lightViewPosition - p_pixelP);
#endif
    float diff = max(dot(norm, lightDir), 0.0);
    //vec3 diffuse = u_lightColor * (diff * material.diffuse);
    vec3 diffuse = vec3(1.0) * (diff * material.diffuse);
    
    //Specular Lighting:
#if DIRECTIONALLIGHT == 1
    vec3 specular = vec3(0);
#else
    vec3 viewDir = normalize(vec3(0) - p_pixelP);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //vec3 specular = u_lightColor * (spec * material.specular);
    vec3 specular = vec3(1.0) * (spec * material.specular);
#endif
    
    float adjustedVertexCount = p_connectedVertices;//min(p_connectedVertices, 4);
    float ambientOcclusion = adjustedVertexCount / 4.0;
    vec3 result = (max(ambient + diffuse + specular - ambientOcclusion, 0.25)) * pixel.xyz;
    ambient = vec3(1);
    result = (max(ambient - ambientOcclusion, 0)) * pixel.xyz;
    #if 0
    const float e0 = 3.99;
    const float e1 = 2;
    const float e2 = 1;
    if (p_connectedVertices > e0)
        result = vec3( adjustedVertexCount - e0, 0, 0 );
    else if (p_connectedVertices > e1)
        result = vec3( 0, adjustedVertexCount - e1, 0 );
    else if (p_connectedVertices > e2)
        result = vec3( 0, 0, adjustedVertexCount);
    else
        result = vec3( 0, 0, 0 );
        #endif
    //result = vec3(p_connectedVertices / 2);
    color = vec4(result, pixel.a);
    //color = pixel;

    //color.xyz = p_normal;
    //color.a = 1;
    //color.xyz = vec3(p_uv * 16, 0);
}
