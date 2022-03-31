#version 420 core
layout(location = 0) in vec3 v_p;
layout(location = 1) in uint v_n;
layout(location = 2) in uint v_ao;
layout(location = 3) in uvec4 v_rgba;

out vec4 p_color;
out vec3 p_normal;
out float p_connectedVertices;
out mat4 p_view;

//uniform mat4 u_modelMove;
//uniform mat4 u_modelRotate;
uniform mat4 u_rotate;
uniform mat4 u_model;
uniform mat4 u_perspective;
uniform mat4 u_view;
uniform mat4 u_toModel;
uniform mat4 u_fromModel;

const vec3 faceNormals[6] = vec3[6](
    vec3(  1.0,  0.0,  0.0 ),
    vec3( -1.0,  0.0,  0.0 ),
    vec3(  0.0,  1.0,  0.0 ),
    vec3(  0.0, -1.0,  0.0 ),
    vec3(  0.0,  0.0,  1.0 ),
    vec3(  0.0,  0.0, -1.0 )
);

void main()
{
    //gl_Position = u_perspective * u_view * u_model * u_fromModel * u_rotate * u_toModel * u_modelMove * u_modelRotate * vec4(v_position, 1.0);
    gl_Position = u_perspective * u_view * u_model * u_fromModel * u_rotate * u_toModel * vec4(v_p, 1.0);
    //p_color = vec4(v_rgba);
#if 0
    p_color = v_rgba;
#else
    p_color.r = float(v_rgba.r) / 255;
    p_color.g = float(v_rgba.g) / 255;
    p_color.b = float(v_rgba.b) / 255;
    p_color.a = float(v_rgba.a) / 255;
#endif
    p_connectedVertices = float(v_ao);
    p_view = u_view;
    p_normal = (u_view * u_rotate * vec4(faceNormals[v_n], 0)).xyz;
    //p_normal = vec3(float(v_n));
    //p_normal = faceNormals[v_n];
}
