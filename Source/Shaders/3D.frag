#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;
in vec3 p_pixelP;

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
    vec4 pixel = texture(sampler, p_uv);
    //Ambient Lighting:
    vec3 ambient = vec3(1.0) * material.ambient;
    //vec3 ambient = u_lightColor * material.ambient;

    //Diffuse Lighting:
    vec3 norm = normalize(p_normal);
    vec3 lightDir = normalize(u_lightP- p_pixelP);
    float diff = max(dot(norm, lightDir), 0.0);
    //vec3 diffuse = u_lightColor * (diff * material.diffuse);
    vec3 diffuse = vec3(1.0) * (diff * material.diffuse);
    
    //Specular Lighting:
    vec3 viewDir = normalize(u_cameraP - p_pixelP);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //vec3 specular = u_lightColor * (spec * material.specular);
    vec3 specular = vec3(1.0) * (spec * material.specular);

    vec3 result = (ambient + diffuse + specular) * pixel.xyz;
    color = vec4(result, pixel.a);

    //color.xyz = p_normal;
    //color.a = 1;
}
