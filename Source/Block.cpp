#include "Block.h"

void Block::Render()
{
    RenderBlock(this);
}

struct VertexFace {
	Vertex a,b,c,d;
};

static const VertexFace cubeFaces[+Face::Count] = {

	{
	{ {  1.0f,  1.0f,     0 }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
	{ {  1.0f,     0,     0 }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
	{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
	{ {  1.0f,     0,  1.0f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } }, // +x
	},

	{
	{ { 0,  1.0f,     0 }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },    // top right
	{ { 0,     0,     0 }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },    // Top Left
	{ { 0,  1.0f,  1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },    // bot right
	{ { 0,     0,  1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },    // -x bottom Left
	},

	{
	{ {     0,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } }, // +y
	{ {     0,  1.0f,     0 }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
	{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
	{ {  1.0f,  1.0f,     0 }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
	},

	{
	{ {     0,     0,  1.0f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // -y
	{ {     0,     0,     0 }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
	{ {  1.0f,     0,  1.0f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
	{ {  1.0f,     0,     0 }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
	},

	{
	{ {     0,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } }, // z
	{ {     0,     0,  1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
	{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
	{ {  1.0f,     0,  1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
	},

	{
	{ {     0,  1.0f,     0 }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // -z
	{ {     0,     0,     0 }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
	{ {  1.0f,  1.0f,     0 }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
	{ {  1.0f,     0,     0 }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
	},
};
static uint32 cubeIndices[36] = {};

struct BlockSprites
{
	uint32 faceSprites[+Face::Count] = {
		defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation,
		defaultSpriteLocation, defaultSpriteLocation, defaultSpriteLocation };
};

static BlockSprites faceSprites[+BlockType::Count];

void SetBlockSprites()
{
	for (uint32 i = 0; i < +Face::Count; i++)
	{
		faceSprites[+BlockType::Grass].faceSprites[i] = 3;
	}
	faceSprites[+BlockType::Grass].faceSprites[+Face::Top] = 0;
	faceSprites[+BlockType::Grass].faceSprites[+Face::Bot] = 2;

	for (uint32 i = 0; i < +Face::Count; i++)
	{
		faceSprites[+BlockType::Stone].faceSprites[i] = 1;
	}

	for (uint32 i = 0; i < +Face::Count; i++)
	{
		faceSprites[+BlockType::IronBlock].faceSprites[i] = 22;
	}
}

	
Vec3Int Vec3ToVec3Int(Vec3 a)
{
	return { static_cast<int32>(a.x), static_cast<int32>(a.y), static_cast<int32>(a.z) };
}

Vec3 Vec3IntToVec3(Vec3Int a)
{
	return { static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) };
}

Vec3Int Chunk::BlockPosition()
{
	return { p.x* static_cast<int32>(CHUNK_X), p.y* static_cast<int32>(CHUNK_Y), p.z* static_cast<int32>(CHUNK_Z) };
}


//uint32 GetBlockWeight(BlockType b, uint32 y)
//{
//	BlockType bt = BlockType::Empty;
//	if (y > CHUNK_Y / 2)
//	{
//		bt = BlockType::Empty;
//	}
//	else
//	{
//		if (y > CHUNK_Y / 4)
//		{
//			bt = BlockType::Grass;
//		}
//		else
//		{
//			uint32 random = RandomU32(+BlockType::Stone, static_cast<uint32>(arrsize(options)));
//			bt = options[random];
//		}
//	}
//
//	switch (b)
//	{
//	case BlockType::Empty:
//	{
//		if (y > CHUNK_Y / 2)
//		{
//			return 10;
//		}
//		else
//		{
//			return 1;
//		}
//		break;
//	}
//
//	case BlockType::Grass:
//	{
//		if (y > CHUNK_Y / 2)
//		{
//			return 0;
//		}
//		else
//		{
//			return 1;
//		}
//		break;
//	}
//
//	case BlockType::Stone:
//	{
//		if (y > CHUNK_Y / 2)
//		{
//			return 0;
//		}
//		else if (y > CHUNK_Y / 4)
//		{
//			return 1;
//		}
//		else
//		{
//			return 3;
//		}
//		break;
//	}
//
//	case BlockType::IronBlock:
//	{
//		if (y > CHUNK_Y / 2)
//		{
//			return 0;
//		}
//		else if (y > CHUNK_Y / 4)
//		{
//			return 1;
//		}
//		else
//		{
//			return 3;
//		}
//		break;
//	}
//	default:
//	{
//		assert(false);
//	}
//	}
//}
//
//BlockType GetBlockType(uint32 y)
//{
//	for ()
//	int sum_of_weight = 0;
//	for (int i = 0; i < num_choices; i++) {
//		sum_of_weight += choice_weight[i];
//	}
//	int rnd = random(sum_of_weight);
//	for (int i = 0; i < num_choices; i++) {
//		if (rnd < choice_weight[i])
//			return i;
//		rnd -= choice_weight[i];
//	}
//	assert(!"should never get here");
//}

void Chunk::SetBlocks()
{
	BlockType options[] = {
		BlockType::Empty,
		BlockType::Grass,
		BlockType::Stone,
		BlockType::IronBlock,
	};



	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 y = 0; y < CHUNK_Y; y++)
		{
			for (int32 z = 0; z < CHUNK_Z; z++)
			{

				BlockType bt = BlockType::Empty;
				if (y > CHUNK_Y / 2)
				{
					bt = BlockType::Empty;
				}
				else
				{
					if (y > CHUNK_Y / 3)
					{
						bt = BlockType::Grass;
					}
					else if (y > CHUNK_Y / 4)
					{
						uint32 random = RandomU32(+BlockType::Grass, static_cast<uint32>(arrsize(options)));
						bt = options[random];
					}
					else
					{
						uint32 random = RandomU32(+BlockType::Stone, static_cast<uint32>(arrsize(options)));
						bt = options[random];
					}
				}
				arr[x][y][z] = bt;
			}
		}
	}
}

void Chunk::BuildChunkVertices()
{
	SetBlockSprites();
	faceVertices.clear();
	indices.clear();
	uint32 baseIndex = 0;
	Vec3Int realP = BlockPosition();
	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 y = 0; y < CHUNK_Y; y++)
		{
			for (int32 z = 0; z < CHUNK_Z; z++)
			{
				for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
				{
					BlockType currentBlockType = arr[x][y][z];
					if (currentBlockType == BlockType::Empty)
						continue;

					Vec3Int vf = Vec3ToVec3Int(cubeFaces[faceIndex].a.n);
					int32 xReal = x + vf.x;
					int32 yReal = y + vf.y;
					int32 zReal = z + vf.z;

					bool outOfBounds = (xReal >= CHUNK_X || yReal >= CHUNK_Y || zReal >= CHUNK_Z ||
						xReal < 0 || yReal < 0 || zReal < 0);

					if (outOfBounds || arr[xReal][yReal][zReal] == BlockType::Empty)
					{
						VertexFace f = cubeFaces[faceIndex];
						Vec3 offset = { static_cast<float>(x + realP.x), static_cast<float>(y + realP.y), static_cast<float>(z + realP.z) };

						f.a.p += offset;
						f.b.p += offset;
						f.c.p += offset;
						f.d.p += offset;

						Rect s = GetRectFromSprite(faceSprites[+currentBlockType].faceSprites[faceIndex]);
						const uint32 size = blocksPerRow * pixelsPerBlock;
						float iblx = Clamp(s.botLeft.x  / size, 0.0f, 1.0f);
						float ibly = Clamp(s.botLeft.y  / size, 0.0f, 1.0f);
						float itrx = Clamp(s.topRight.x / size, 0.0f, 1.0f);
						float itry = Clamp(s.topRight.y / size, 0.0f, 1.0f);
					
						f.a.uv = { iblx, itry }; //Bot Left
						f.b.uv = { iblx, ibly }; //Top Left
						f.c.uv = { itrx, itry }; //Bot Right
						f.d.uv = { itrx, ibly }; //Top Right

						faceVertices.push_back(f.a);
						faceVertices.push_back(f.b);
						faceVertices.push_back(f.c);
						faceVertices.push_back(f.d);

						if (faceIndex == 1 || faceIndex == 3 || faceIndex == 4)
						{
							indices.push_back(baseIndex + 0);
							indices.push_back(baseIndex + 1);
							indices.push_back(baseIndex + 2);
							indices.push_back(baseIndex + 1);
							indices.push_back(baseIndex + 3);
							indices.push_back(baseIndex + 2);
						}
						else
						{
							indices.push_back(baseIndex + 0);
							indices.push_back(baseIndex + 2);
							indices.push_back(baseIndex + 1);
							indices.push_back(baseIndex + 1);
							indices.push_back(baseIndex + 2);
							indices.push_back(baseIndex + 3);
						}
						baseIndex += 4; //Amount of vertices
					}
				}
			}
		}
	}

}

void Chunk::UploadChunk()
{
	vertexBuffer.Upload(faceVertices.data(), faceVertices.size());
	indexBuffer.Upload(indices.data(), indices.size());
}

void Chunk::RenderChunk()
{
	vertexBuffer.Bind();
	indexBuffer.Bind();
    g_renderer.textures[Texture::Minecraft]->Bind();
    g_renderer.programs[+Shader::Simple3D]->UseShader();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    //gb_mat4_translate(&transform, Vec3IntToVec3(p));
	gb_mat4_identity(&transform);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
    sp->UpdateUniformMat4( "u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4( "u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4( "u_model",       1, false, transform.e);

	sp->UpdateUniformVec3( "u_lightColor",	    1,  g_light.c.e);
	sp->UpdateUniformVec3( "u_lightP",          1,  g_light.p.e);
	sp->UpdateUniformVec3( "u_cameraP",         1,  g_camera.p.e);

	Material material;
	material.ambient = { 0.02f, 0.02f, 0.02f };
	material.diffuse = { 1.0f, 1.0f, 1.0f };
	material.specular = { 0.4f, 0.4f,  0.4f };
	material.shininess = 32;//0.78125f;
	sp->UpdateUniformVec3("material.ambient", 1,	material.ambient.e);
	sp->UpdateUniformVec3( "material.diffuse",  1,  material.diffuse.e);
	sp->UpdateUniformVec3( "material.specular", 1,  material.specular.e);
	sp->UpdateUniformFloat("material.shininess",    material.shininess);


    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}
