#include "Block.h"
#include "WinInterop.h"
#include "Noise.h"


struct VertexFace {
	Vertex_Chunk a,b,c,d;
};

Vec3 faceNormals[] = {

{  1.0f,  0.0f,  0.0f },
{ -1.0f,  0.0f,  0.0f },
{  0.0f,  1.0f,  0.0f },
{  0.0f, -1.0f,  0.0f },
{  0.0f,  0.0f,  1.0f },
{  0.0f,  0.0f, -1.0f },
};

struct BlockSprites
{
	uint8 faceSprites[+Face::Count] = {
		defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
		defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
};

static BlockSprites faceSprites[+BlockType::Count];

void SetMultipleBlockSprites(BlockType bt, uint32 v)
{
	for (uint32 i = 0; i < +Face::Count; i++)
	{
		faceSprites[+bt].faceSprites[i] = v;
	}
}
void SetBlockSprites()
{
	SetMultipleBlockSprites(BlockType::Grass, 3);
	faceSprites[+BlockType::Grass].faceSprites[+Face::Top] = 0;
	faceSprites[+BlockType::Grass].faceSprites[+Face::Bot] = 2;

	SetMultipleBlockSprites(BlockType::Stone, 1);
	SetMultipleBlockSprites(BlockType::IronBlock, 22);
	SetMultipleBlockSprites(BlockType::Dirt, 2);
}


Vec3Int Chunk::BlockPosition()
{
	return { p.x* static_cast<int32>(CHUNK_X), p.y* static_cast<int32>(CHUNK_Y), p.z* static_cast<int32>(CHUNK_Z) };
}

Vec3Int ToChunkPosition(Vec3 p)
{

	Vec3Int result = { static_cast<int32>(p.x) / static_cast<int32>(CHUNK_X),
					   static_cast<int32>(p.y) / static_cast<int32>(CHUNK_Y),
					   static_cast<int32>(p.z) / static_cast<int32>(CHUNK_Z) };
	return result;
}


void Chunk::SetBlocks()
{
	BlockType options[] = {
		BlockType::Empty,
		BlockType::Grass,
		BlockType::Dirt,
		BlockType::Stone,
		BlockType::IronBlock,
	};
	blocks = std::make_unique<ChunkData>();

	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 z = 0; z < CHUNK_Z; z++)
		{
			Vec2 blockP = { static_cast<float>(x), static_cast<float>(z) };
			Vec3Int chunkBlockP = BlockPosition();

			Vec2 blockRatio = { chunkBlockP.x + blockP.x, chunkBlockP.z + blockP.y };

#if NOISETYPE == 2
			//blockRatio /= 200;
			//int32 yTotal = Max(static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 10);
			blockRatio /= 100;
			int32 yTotal = Clamp<uint32>(static_cast<int32>(Noise(blockRatio, 1.0f) * CHUNK_Y), 10, CHUNK_Y - 1);
#elif NOISETYPE == 4
			blockRatio /= 100;
			int32 yTotal = (static_cast<int32>(Noise(blockRatio) * CHUNK_Y), 80);
#else
			static_assert(false, "Need to set noise implimentation variabls in SetBlocks()");
#endif
			assert(yTotal >= 0 && yTotal < CHUNK_Y);


			for (int32 y = 0; y < yTotal; y++)
			{

				BlockType bt = BlockType::Empty;
				//if (y > CHUNK_Y / 2)
				//{
				//	bt = BlockType::Empty;
				//}
				//else
				{
					if (y == yTotal - 1)
					{
						bt = BlockType::Grass;
					}
					else if (y > yTotal - 4)
					{
                        bt = BlockType::Dirt;
						//uint32 random = RandomU32(+BlockType::Grass, static_cast<uint32>(arrsize(options)));
					}
					else
					{
						//uint32 random = RandomU32(+BlockType::Stone, static_cast<uint32>(arrsize(options)));
						//uint32 random = RandomU32(0, static_cast<uint32>(arrsize(options)));
						//bt = options[random];
						bt = BlockType::Stone;
					}
				}
				blocks->e[x][y][z] = bt;
			}
		}
	}
	//blocks->e[CHUNK_X - 1][CHUNK_Y - 1][CHUNK_Z - 1] = BlockType::Grass;
}

Vec3Int GetBlockPosFromIndex(uint16 index)
{
    int32 blockY =  index / CHUNK_Y;
    int32 duplicateMath = (index - blockY * CHUNK_Y);
    int32 blockZ = (duplicateMath) / CHUNK_Z;
    int32 blockX =  duplicateMath - blockZ * CHUNK_Z;

	return { blockX, blockY, blockZ };
}

BlockType Chunk::GetBlock(Vec3Int a)
{
	if (a.x >= CHUNK_X || a.y >= CHUNK_Y || a.z >= CHUNK_Z)
		return BlockType::Empty;

	return blocks->e[a.x][a.y][a.z];
}

void Chunk::BuildChunkVertices()
{
	SetBlockSprites();
	faceVertices.clear();
	uploadedIndexCount = 0;
	Vec3Int realP = BlockPosition();
	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 y = 0; y < CHUNK_Y; y++)
		{
			for (int32 z = 0; z < CHUNK_Z; z++)
			{
				for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
				{
					BlockType currentBlockType = blocks->e[x][y][z];
					if (currentBlockType == BlockType::Empty)
						continue;

					Vec3Int vf = Vec3ToVec3Int(faceNormals[faceIndex]);
					int32 xReal = x + vf.x;
					int32 yReal = y + vf.y;
					int32 zReal = z + vf.z;

					bool outOfBounds = (xReal >= CHUNK_X || yReal >= CHUNK_Y || zReal >= CHUNK_Z ||
						xReal < 0 || yReal < 0 || zReal < 0);

					if (outOfBounds || blocks->e[xReal][yReal][zReal] == BlockType::Empty)
					{
						VertexFace f = {};// = cubeFaces[faceIndex];
						Vec3 offset = { static_cast<float>(x + realP.x), static_cast<float>(y + realP.y), static_cast<float>(z + realP.z) };

						f.a.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
						f.b.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
						f.c.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;
						f.d.blockIndex = z * CHUNK_Z + y * CHUNK_Y + x;

						f.a.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
						f.b.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
						f.c.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];
						f.d.spriteIndex = faceSprites[+currentBlockType].faceSprites[faceIndex];

						f.a.n = faceIndex;
						f.b.n = faceIndex;
						f.c.n = faceIndex;
						f.d.n = faceIndex;

						//f.a.connectedVertices = ;
						//f.b.connectedVertices = ;
						//f.c.connectedVertices = ;
						//f.d.connectedVertices = ;

						faceVertices.push_back(f.a);
						faceVertices.push_back(f.b);
						faceVertices.push_back(f.c);
						faceVertices.push_back(f.d);

						uploadedIndexCount += 6;
					}
				}
			}
		}
	}
	/*
	This is the solution array taht is just for refernce now
		//{-1, -1, -1 },
		{ 0, -1, -1 },
		//{ 1, -1, -1 },
		{-1, -1,  0 },
		//{ 0, -1,  0 },
		{ 1, -1,  0 },
		//{-1, -1,  1 },
		{ 0, -1,  1 },
		//{ 1, -1,  1 },
		{-1,  0, -1 },
		//{ 0,  0, -1 },
		{ 1,  0, -1 },
		//{-1,  0,  0 },
		//{ 0,  0,  0 },
		//{ 1,  0,  0 },
		{-1,  0,  1 },
		//{ 0,  0,  1 },
		{ 1,  0,  1 },
		//{-1,  1, -1 },
		{ 0,  1, -1 },
		//{ 1,  1, -1 },
		{-1,  1,  0 },
		//{ 0,  1,  0 },
		{ 1,  1,  0 },
		//{-1,  1,  1 },
		{ 0,  1,  1 },
		//{ 1,  1,  1 },
	*/

	Vec3Int vertexAOBlocks[] = {
		{ 0, -1, -1 },
		{-1, -1,  0 },
		{ 1, -1,  0 },
		{ 0, -1,  1 },
		{-1,  0, -1 },
		{ 1,  0, -1 },
		{-1,  0,  1 },
		{ 1,  0,  1 },
		{ 0,  1, -1 },
		{-1,  1,  0 },
		{ 1,  1,  0 },
		{ 0,  1,  1 },
	};

#define TESTING 2
	uint32 i = 0;
	for (Vertex_Chunk& vert : faceVertices)
	{
		i++;
		i = i % 4;
		Vec3Int vf = Vec3ToVec3Int(faceNormals[vert.n]); //{0, 1, 0}
		Vec3Int avf = Abs(vf);
		Vec3Int blockP = GetBlockPosFromIndex(vert.blockIndex);
		//blocks->e[x][y][z] = bt;
		//vert.connectedVertices = 2;

#if TESTING == 2


		if (vf.x == -1 || vf.x == 1)
		{

			if (GetBlock(blockP + avf + Vec3Int({ 0, 1, 0 })) != BlockType::Empty)
			{
				if (i == 0 || i == 2)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, 0, 1 })) != BlockType::Empty)
			{
				if (i == 2 || i == 3)
				{
					vert.connectedVertices++;
				}
			}

			else if (GetBlock(blockP + avf + Vec3Int({ 0, -1, 0 })) != BlockType::Empty)
			{
				if (i == 3 || i == 1)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, 0, -1 })) != BlockType::Empty)
			{
				if (i == 1 || i == 0)
				{
					vert.connectedVertices++;
				}
			}
		}
		else if (vf.y == -1 || vf.y == 1)
		{

			if (GetBlock(blockP + avf + Vec3Int({ 1, 0, 0 })) != BlockType::Empty)
			{
				if (i == 1 || i == 0)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, 0, 1 })) != BlockType::Empty)
			{
				if (i == 0 || i == 2)
				{
					vert.connectedVertices++;
				}
			}

			else if (GetBlock(blockP + avf + Vec3Int({ -1, 0, 0 })) != BlockType::Empty)
			{
				if (i == 2 || i == 3)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, 0, -1 })) != BlockType::Empty)
			{
				if (i == 1 || i == 3)
				{
					vert.connectedVertices++;
				}
			}
		}
		else if (vf.z == -1 || vf.z == 1)
		{

			if (GetBlock(blockP + avf + Vec3Int({ 1, 0, 0 })) != BlockType::Empty)
			{
				if (i == 0 || i == 2)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, 1, 0 })) != BlockType::Empty)
			{
				if (i == 2 || i == 3)
				{
					vert.connectedVertices++;
				}
			}

			else if (GetBlock(blockP + avf + Vec3Int({ -1, 0, 0 })) != BlockType::Empty)
			{
				if (i == 3 || i == 1)
				{
					vert.connectedVertices++;
				}
			}
			else if (GetBlock(blockP + avf + Vec3Int({ 0, -1, 0 })) != BlockType::Empty)
			{
				if (i == 1 || i == 0)
				{
					vert.connectedVertices++;
				}
			}
		}




#elif TESTING == 1
		uint32 normalDimensionIndex;
		if (vf.x != 0)
		{
			normalDimensionIndex = 0;
		}
		else if (vf.y != 0)
		{
			normalDimensionIndex = 1;
		}
		else if (vf.z != 0)
		{
			normalDimensionIndex = 2;
		}
		else
			assert(Distance(Vec3IntToVec3(vf), { 0,0,0 }) != 0);

		for (int32 i = 0; i < arrsize(vertexAOBlock); i++)
		{
			if (vertexAOBlocks[i].e[normalDimensionIndex] == vf.e[normalDimensionIndex])
			{
				if (vertexAOBlocks)
			}

		}
#else
		for (int32 x = -1; x <= 1; x++)
		{
			for (int32 y = -1; y <= 1; y++)
			{
				for (int32 z = -1; z <= 1; z++)
				{
					Vec3Int newVec = { x, y, z };
					Vec3Int noNorm = newVec - HadamardProduct(newVec, avf);
					Vec3Int blockCheckP = blockP + noNorm;

				}
			}
		}
			//vert->connectedVertices++;
#endif
	}

	flags &= ~(CHUNK_MODIFIED | CHUNK_LOADING);
	flags |= CHUNK_LOADED;
}


void Chunk::UploadChunk()
{
	vertexBuffer.Upload(faceVertices.data(), faceVertices.size());
	std::vector<Vertex_Chunk> faces;
	faceVertices.swap(faces);
	flags |= CHUNK_UPLOADED;
}

void PreChunkRender()
{
	assert(g_renderer.chunkIB);
	if (g_renderer.chunkIB)
		g_renderer.chunkIB->Bind();
	else
		return;

    g_renderer.programs[+Shader::Simple3D]->UseShader();
	g_renderer.spriteTextArray->Bind();

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    //gb_mat4_translate(&transform, Vec3IntToVec3(p));
	gb_mat4_identity(&transform);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
    sp->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4("u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4("u_model",       1, false, transform.e);

#if DIRECTIONALLIGHT == 1
	sp->UpdateUniformVec3("u_directionalLight_d",  1,  g_light.d.e);
#else
	sp->UpdateUniformVec3("u_lightColor",  1,  g_light.c.e);
	sp->UpdateUniformVec3("u_lightP",      1,  g_light.p.e);
#endif
	sp->UpdateUniformVec3("u_cameraP",     1,  g_camera.p.e);

	sp->UpdateUniformUint8("u_CHUNK_X", CHUNK_X);
	sp->UpdateUniformUint8("u_CHUNK_Y", CHUNK_Y);
	sp->UpdateUniformUint8("u_CHUNK_Z", CHUNK_Z);

	Material material;
	material.ambient = { 0.02f, 0.02f, 0.02f };
	material.diffuse = { 1.0f, 1.0f, 1.0f };
	material.specular = { 0.4f, 0.4f,  0.4f };
	material.shininess = 32;//0.78125f;
	sp->UpdateUniformVec3("material.ambient",   1,	material.ambient.e);
	sp->UpdateUniformVec3( "material.diffuse",  1,  material.diffuse.e);
	sp->UpdateUniformVec3( "material.specular", 1,  material.specular.e);
	sp->UpdateUniformFloat("material.shininess",    material.shininess);

}

void Chunk::RenderChunk()
{
	vertexBuffer.Bind();

    glVertexAttribIPointer(0, 1, GL_UNSIGNED_SHORT, sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, blockIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, spriteIndex));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Chunk), (void*)offsetof(Vertex_Chunk, connectedVertices));
    glEnableVertexArrayAttrib(g_renderer.vao, 3);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
	sp->UpdateUniformVec3("u_chunkP",      1,  Vec3IntToVec3(BlockPosition()).e);

    glDrawElements(GL_TRIANGLES, (GLsizei)uploadedIndexCount, GL_UNSIGNED_INT, 0);
}
