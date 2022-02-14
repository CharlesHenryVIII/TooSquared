#version 420 core
//Color Texture:
layout (binding = 0) uniform sampler2D sampler;
//Depth Peel:
layout (binding = 1) uniform sampler2D depthTexture;
//Opaque Depth Pass:
layout (binding = 2) uniform sampler2D opaqueDepthTexture;

uniform uint u_passCount;

in vec4 p_color;
in vec2 p_uv;
in vec4 gl_FragCoord;

out vec4 color;

void main()
{
    vec4 depthTexel        = texelFetch(depthTexture,         ivec2(gl_FragCoord.xy), 0);
    vec4 opaqueDepthTexel  = texelFetch(opaqueDepthTexture,   ivec2(gl_FragCoord.xy), 0);
#if 1
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
#else
    if (gl_FragCoord.z <= texture(depthTexture, vec2(gl_FragCoord.x, gl_FragCoord.y)).z) 
        discard; //Manually performing the GL_GREATER depth test for each pixel
#endif

    vec4 tempColor = p_color * texture(sampler, p_uv);
    if (tempColor.a == 0)
        discard;
    color = tempColor;
}
