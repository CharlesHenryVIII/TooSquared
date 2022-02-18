#version 420 core
//Color Texture:
layout (binding = 0) uniform sampler2DArray sampler;
//Depth Peel:
layout (binding = 1) uniform sampler2D depthTexture;
//Opaque Depth Pass:
layout (binding = 2) uniform sampler2D opaqueDepthTexture;

uniform uint u_passCount;

in vec2 p_uv;
in vec4 gl_FragCoord;
flat in float p_depth;

out vec4 color;

void main()
{
    vec4 depthTexel        = texelFetch(depthTexture,         ivec2(gl_FragCoord.xy), 0);
    vec4 opaqueDepthTexel  = texelFetch(opaqueDepthTexture,   ivec2(gl_FragCoord.xy), 0);
    if (u_passCount == 1)
    {
        if ((gl_FragCoord.z >= opaqueDepthTexel.r))
            discard;
    }
    else if (u_passCount > 1) //I think I can get away with removing this if statement
    {
        if ((gl_FragCoord.z >= opaqueDepthTexel.r) || (gl_FragCoord.z <= (depthTexel.r)))
            discard;
    }

    vec4 tempColor = texture(sampler, vec3(p_uv, p_depth));
    if (tempColor.a == 0)
        discard;

    if (u_passCount == 0)
        color = vec4(tempColor.xyz, 1);
    else
        color = tempColor;
}
