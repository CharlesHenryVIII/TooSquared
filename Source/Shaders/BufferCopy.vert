#version 330
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_n;

out vec2 f_uv;

void main()
{
	f_uv = v_uv;
	gl_Position = vec4(v_position, 1);
}
