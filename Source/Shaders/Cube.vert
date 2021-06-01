#version 420 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_model;
uniform vec3 u_scale;
uniform vec4 u_color;

out vec4 p_color;
out vec2 p_uv;

const vec2 faceUV[4] = vec2[4](
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);

void main()
{
    gl_Position = u_perspective * u_view * u_model * vec4(v_position * u_scale, 1.0);
    p_color = u_color;
    p_uv = v_uv;
}
