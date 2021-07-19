#version 420 core
layout(location = 0) out vec4 accum;
layout(location = 1) out float reveal;

uniform sampler2D sampler;

in vec4 p_color;
in vec2 p_uv;

out vec4 color;

void main()
{
    if ()
    vec4 color = p_color * texture(sampler, p_uv);

    //Weighted-Blend Implimentation:
    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * 
                         pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
    // store pixel color accumulation
    accum = vec4(color.rgb * color.a, color.a) * weight;
    // store pixel revealage threshold
    reveal = color.a;

    if (color.a == 0)
        discard;
}
