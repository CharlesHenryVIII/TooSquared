#version 330 core
#extension GL_EXT_texture_array : enable
uniform sampler2DArray sampler;

in vec2 p_uv;
flat in float p_depth;
in vec3 p_normal;
in vec3 p_pixelP;
in mat4 p_view;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

uniform vec3 u_lightColor;
uniform vec3 u_lightP;
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
    vec3 lightViewPosition = (p_view * vec4(u_lightP, 1)).xyz;
    vec3 lightDir = normalize(lightViewPosition - p_pixelP);
    float diff = max(dot(norm, lightDir), 0.0);
    //vec3 diffuse = u_lightColor * (diff * material.diffuse);
    vec3 diffuse = vec3(1.0) * (diff * material.diffuse);
    
    //Specular Lighting:
    vec3 viewDir = normalize(vec3(0) - p_pixelP);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //vec3 specular = u_lightColor * (spec * material.specular);
    vec3 specular = vec3(1.0) * (spec * material.specular);

    vec3 result = (max(ambient + diffuse + specular, 0.25)) * pixel.xyz;
    color = vec4(result, pixel.a);
    //color = pixel;

    //color.xyz = p_normal;
    //color.a = 1;
    //color.xyz = vec3(p_uv * 16, 0);
}
