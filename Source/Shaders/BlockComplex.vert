#version 420 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in uint v_index;

uniform mat4 u_rotate;
uniform mat4 u_model;
uniform mat4 u_perspective;
uniform mat4 u_view;

out vec2 p_uv;
flat out float p_depth;

void main()
{
    gl_Position = u_perspective * u_view * u_model * u_rotate * vec4(v_position, 1.0);
    p_uv = v_uv;
    p_depth = v_index;
}
