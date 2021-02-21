#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;

out vec4 color;

void main()
{
    color = texture(sampler, p_uv);
    //color.xyz = p_normal;
    //color.a = 1;
}
