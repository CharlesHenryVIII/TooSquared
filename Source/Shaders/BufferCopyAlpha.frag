#version 420 core
//uniform sampler2D sampler;
layout (binding = 0) uniform sampler2D sourceSampler;

layout (binding = 1) uniform sampler2D destSampler;

in vec2 f_uv;
out vec4 color;

void main()
{
    vec4 s = texture(sourceSampler, f_uv);
    vec4 d = texture(destSampler,   f_uv);
    vec4 sf = s * s.a;
    vec4 df = d * (1 - s.a);
    color = sf + df;

    if (s.a == 1 || d.a == 1)
        color.a = 1;
}
