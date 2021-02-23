#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;
in vec3 p_pixelP;

uniform vec3 u_lightColor;
uniform vec3 u_lightP;
uniform vec3 u_cameraP;
uniform float u_reflect;

out vec4 color;


void main()
{
    vec4 pixel = texture(sampler, p_uv);
    //Ambient Lighting:
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * u_lightColor;

    //Diffuse Lighting:
    vec3 norm = normalize(p_normal);
    vec3 lightDir = normalize(u_lightP- p_pixelP);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor;
    //vec3 result = (ambient + diffuse) * pixel.xyz;
    
    //Specular Lighting:
    vec3 viewDir = normalize(u_cameraP - p_pixelP);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = u_reflect * spec * u_lightColor;
    vec3 result = (ambient + diffuse + specular) * pixel.xyz;
    color = vec4(result, pixel.a);

    //color = vec4(result, 1.0);
    //color = vec4(realColor.rgb * clamp(1.3 - gl_FragCoord.z, 0, 1), realColor.a);
    //color.xyz = p_normal;
    //color.a = 1;
}
