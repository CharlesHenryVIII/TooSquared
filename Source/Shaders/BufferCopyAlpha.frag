#version 420 core
//uniform sampler2D sampler;
layout (binding = 0) uniform sampler2D sampler;

//layout (binding = 1) uniform sampler2D outputBuffer;

in vec2 f_uv;
out vec4 color;

void main()
{
    vec4 texColor = texture(sampler, f_uv);
    //vec4 outColor = texture(outputBuffer, f_uv);
    color = texColor;
    //color.xyz = vec3(1);
    color.a;
    //color.a = max(outColor.a, texColor.a);
    //color.r = 1;
    //color.a = 0.5;
    //color.xyz = vec3(texColor.z);
    //color.r = 1;
    //color.a = 0.5;
    //color.xyzw = vec4(texColor.a);
    //color.xyz = vec3(texColor.a);
    //color = vec4(texColor.rgb, 0.8);
}
