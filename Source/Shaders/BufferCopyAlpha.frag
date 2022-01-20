#version 420 core
//uniform sampler2D sampler;
layout (binding = 0) uniform sampler2D sampler;

in vec2 f_uv;
out vec4 color;

void main()
{
    vec4 texColor = texture(sampler, f_uv);
    color = texColor;
    //color.xyz = vec3(texColor.a);
    //color = vec4(texColor.rgb, 0.8);
}
