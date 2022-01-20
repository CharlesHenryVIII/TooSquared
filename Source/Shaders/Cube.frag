#version 420 core
//Color Texture:
layout (binding = 0) uniform sampler2D sampler;
//Depth Peel:
layout (binding = 1) uniform sampler2D depthTexture;

in vec4 p_color;
in vec2 p_uv;
in vec4 gl_FragCoord;

out vec4 color;

void main()
{
    if (gl_FragCoord.z <= texture(depthTexture, vec2(gl_FragCoord.x, gl_FragCoord.y)).z) 
        discard; //Manually performing the GL_GREATER depth test for each pixel

    vec4 tempColor = p_color * texture(sampler, p_uv);
    if (tempColor.a == 0)
        discard;
    color = tempColor;
}
