#version 420 core
uniform sampler2D sampler;

in vec2 f_uv;
out vec4 color;

void main()
{
    color.xyz = texture(sampler, f_uv).xyz;
    //color.xyz = vec3(1, 1, 1);
    color.a = 1.0;
}
