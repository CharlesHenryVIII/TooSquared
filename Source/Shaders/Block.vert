#version 420 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_scale;
layout(location = 2) in uint v_index;

uniform mat4 u_perspective;
uniform mat4 u_view;

out vec2 p_uv;
flat out float p_depth;

const vec2 faceUV[4] = vec2[4](
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);
mat4 identity = mat4(1.0, 0.0, 0.0, 0.0,
                     0.0, 1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0);

struct VertexFace {
    vec3[4] e;
};

const VertexFace cubeVertices[6] = VertexFace[6](
    // +x
    VertexFace(vec3[4](
	vec3( 1.0,  1.0,  1.0 ),
	vec3( 1.0,    0,  1.0 ), 
	vec3( 1.0,  1.0,    0 ),
	vec3( 1.0,    0,    0 )
    )),

    // -x
    VertexFace(vec3[4](
	vec3(   0,  1.0,    0 ),
	vec3(   0,    0,    0 ),
	vec3(   0,  1.0,  1.0 ),
	vec3(   0,    0,  1.0 )
	)),

    // +y
    VertexFace(vec3[4](
	vec3( 1.0,  1.0,  1.0 ),
	vec3( 1.0,  1.0,    0 ),
	vec3(   0,  1.0,  1.0 ), 
	vec3(   0,  1.0,    0 )
    )),

    // -y
	VertexFace(vec3[4](
	vec3(   0,    0,  1.0 ), 
	vec3(   0,    0,    0 ),
	vec3( 1.0,    0,  1.0 ),
	vec3( 1.0,    0,    0 )
	)),

    // z
	VertexFace(vec3[4](
	vec3(   0,  1.0,  1.0 ), 
	vec3(   0,    0,  1.0 ),
	vec3( 1.0,  1.0,  1.0 ),
	vec3( 1.0,    0,  1.0 )
	)),

    // -z
    VertexFace(vec3[4](
	vec3( 1.0,  1.0,    0 ),
	vec3( 1.0,    0,    0 ),
	vec3(   0,  1.0,    0 ), 
	vec3(   0,    0,    0 )
    ))
);

void main()
{
    mat4 model = identity;
    model[3][0] = v_position.x;
    model[3][1] = v_position.y;
    model[3][2] = v_position.z;
    model[3][3] = 1.0;

    uint relativeIndex = gl_VertexID % 24; //24 vertices uploaded per block
    vec3 baseVertexPosition = cubeVertices[relativeIndex / 4].e[relativeIndex % 4];
    gl_Position = u_perspective * u_view * model * vec4(baseVertexPosition * v_scale, 1.0);
    p_uv = faceUV[gl_VertexID % 4];

    p_depth = v_index;
}
