#version 420 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;

layout(binding = 0) uniform sampler2D s_texture;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_model;
uniform vec3 u_scale;
uniform vec4 u_color;

out vec4 p_color;

void main()
{
    gl_Position = u_perspective * u_view * u_model * vec4(v_position * u_scale, 1.0);
    p_color = u_color * texture(s_texture, v_uv);
}
