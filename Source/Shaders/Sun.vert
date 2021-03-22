#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_n;

out vec2 p_clipspace;

void main()
{
	gl_Position = vec4(v_position, 1);
    p_clipspace = gl_Position.xy;
}
