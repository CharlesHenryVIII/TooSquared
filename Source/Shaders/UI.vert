#version 330 core
layout(location = 0) in vec2 v_p;
layout(location = 1) in vec2 v_uv;

uniform vec2 u_scale;
uniform vec4 u_color;

out vec4 p_color;
out vec2 p_uv;

void main()
{
    gl_Position = vec4(v_p * u_scale, 0, 1);
    p_color = u_color;
}
