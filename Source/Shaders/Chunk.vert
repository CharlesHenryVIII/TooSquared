#version 420 core
layout(location = 0) in uint v_blockIndex;                  //2Byte
layout(location = 1) in uint v_spriteIndex;                 //1Byte
layout(location = 2) in uint v_normalAndConnectedVertices;  //1Byte
//layout(location = 3) in uint v_connectedVertices;         //1Byte

out vec2 p_uv;
flat out float p_depth;
out vec3 p_normal;
out vec3 p_pixelP;
out mat4 p_view;
out float p_connectedVertices;

uniform mat4 u_perspective;
uniform mat4 u_view;
uniform vec3 u_chunkP;
uniform uint u_CHUNK_X;
uniform uint u_CHUNK_Y;
uniform uint u_CHUNK_Z;

const vec3 faceNormals[6] = vec3[6](
    vec3(  1.0,  0.0,  0.0 ),
    vec3( -1.0,  0.0,  0.0 ),
    vec3(  0.0,  1.0,  0.0 ),
    vec3(  0.0, -1.0,  0.0 ),
    vec3(  0.0,  0.0,  1.0 ),
    vec3(  0.0,  0.0, -1.0 )
);

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

const vec2 faceUV[4] = vec2[4](
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);

void main()
{
    uint normalIndex = uint(v_normalAndConnectedVertices & uint(0xF0)) >> 4;
    uint connectedVertices = uint(v_normalAndConnectedVertices & uint(0x0F));

    vec3 faceP = cubeVertices[normalIndex].e[gl_VertexID % 4];

    uint blockY =  v_blockIndex / u_CHUNK_Y;
    uint duplicateMath = (v_blockIndex - blockY * u_CHUNK_Y);
    uint blockZ = (duplicateMath) / u_CHUNK_Z;
    uint blockX =  duplicateMath - blockZ * u_CHUNK_Z;

    vec3 vertexPosition = u_chunkP + vec3(blockX, blockY, blockZ) + faceP;
    gl_Position = u_perspective * u_view  * vec4(vertexPosition, 1.0);

    p_uv = faceUV[gl_VertexID % 4];
    p_depth = v_spriteIndex;
    //p_normal = faceNormals[normalIndex];
    p_normal = (u_view * vec4(faceNormals[normalIndex], 0)).xyz;
    p_pixelP = vec3(u_view * vec4(vertexPosition, 1.0));
    p_view = u_view;

    p_connectedVertices = min(connectedVertices, uint(4));

    //Previous gl_Position assignment
    //gl_Position = u_perspective * u_view * vec4(v_position, 1.0);
    //p_pixelP = vec3(u_view * vec4(v_position, 1.0));
}
