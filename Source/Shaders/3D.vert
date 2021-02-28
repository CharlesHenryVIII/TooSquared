#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in uint v_spriteIndex;
layout(location = 2) in uint v_normal;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 p_uv;
flat out float p_depth;
out vec3 p_normal;
out vec3 p_pixelP;
out mat4 p_view;

const vec3 faceNormals[6] = vec3[6](
    vec3(  1.0,  0.0,  0.0 ),
    vec3( -1.0,  0.0,  0.0 ),
    vec3(  0.0,  1.0,  0.0 ),
    vec3(  0.0, -1.0,  0.0 ),
    vec3(  0.0,  0.0,  1.0 ),
    vec3(  0.0,  0.0, -1.0 )
);

//const vec2 faceUV[6] = vec2[6](
//    vec2( 0, 1),
//    vec2( 0, 0),
//    vec2( 1, 1),
//    vec2( 0, 0),
//    vec2( 1, 0),
//    vec2( 1, 1)
//);

const vec2 faceUV[4] = vec2[4](
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);

void main()
{
    p_uv = faceUV[gl_VertexID % 4];
    p_depth = v_spriteIndex;
    p_normal = faceNormals[v_normal];
    //p_normal = (u_view * u_model * vec4(v_normal, 0)).xyz;
	gl_Position = u_perspective * u_view * u_model * vec4(v_position, 1.0);
    p_pixelP = vec3(u_view * u_model * vec4(v_position, 1.0));
    p_view = u_view;
}
