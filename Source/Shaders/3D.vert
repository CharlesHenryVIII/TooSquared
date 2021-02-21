#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 p_uv;
out vec3 p_normal;

void main()
{
    p_uv = v_uv;
    //p_normal = (u_view * u_model * vec4(v_normal, 0)).xyz;
    p_normal = v_normal;
	gl_Position = u_perspective * u_view * u_model * vec4(v_position, 1.0);
}
