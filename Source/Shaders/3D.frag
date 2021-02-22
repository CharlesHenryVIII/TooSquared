#version 330 core
uniform sampler2D sampler;

in vec2 p_uv;
in vec3 p_normal;

uniform vec3 lightColor;
out vec4 color;

void main()
{
    vec4 realColor = texture(sampler, p_uv);
    color = realColor * vec4(lightColor, 1.0);
    //color = vec4(realColor.rgb * clamp(1.3 - gl_FragCoord.z, 0, 1), realColor.a);
    //color.xyz = p_normal;
    //color.a = 1;
}
