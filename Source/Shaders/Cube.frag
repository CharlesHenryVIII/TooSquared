#version 420 core
uniform sampler2D sampler;

in vec4 p_color;
in vec2 p_uv;

out vec4 color;

void main()
{
    color = p_color * texture(sampler, p_uv);
#if 1
    if (color.a == 0)
        discard;
#endif
}
