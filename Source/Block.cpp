#include "Block.h"
#include "WinInterop.h"


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

Vec3Int ToChunkPosition(Vec3 p)
{

	Vec3Int result = { static_cast<int32>(p.x) / static_cast<int32>(CHUNK_X),
					   static_cast<int32>(p.y) / static_cast<int32>(CHUNK_Y),
					   static_cast<int32>(p.z) / static_cast<int32>(CHUNK_Z) };
	return result;
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
#define BICUBIC
void Chunk::SetBlocks()
{
	BlockType options[] = {
		BlockType::Empty,
		BlockType::Grass,
		BlockType::Dirt,
		BlockType::Stone,
		BlockType::IronBlock,
	};

	//Note good divisor numbers:
	//97
	//263 rolling hills

#ifdef BICUBIC

	Vec3Int _chunkBlockP = BlockPosition();
	Vec3 chunkBlockP = { static_cast<float>(_chunkBlockP.x), static_cast<float>(_chunkBlockP.y), static_cast<float>(_chunkBlockP.z) };
	const float divisor = static_cast<float>(97);

	Vec2 chunk00 = { (chunkBlockP.x - 16.0f) / divisor, (chunkBlockP.z - 16.0f) / divisor };
	Vec2 chunk01 = { (chunkBlockP.x + 00.0f) / divisor, (chunkBlockP.z - 16.0f) / divisor };
	Vec2 chunk02 = { (chunkBlockP.x + 16.0f) / divisor, (chunkBlockP.z - 16.0f) / divisor };
	Vec2 chunk03 = { (chunkBlockP.x + 32.0f) / divisor, (chunkBlockP.z - 16.0f) / divisor };

	Vec2 chunk10 = { (chunkBlockP.x - 16.0f) / divisor, (chunkBlockP.z + 00.0f) / divisor };
	Vec2 chunk11 = { (chunkBlockP.x + 00.0f) / divisor, (chunkBlockP.z + 00.0f) / divisor };
	Vec2 chunk12 = { (chunkBlockP.x + 16.0f) / divisor, (chunkBlockP.z + 00.0f) / divisor };
	Vec2 chunk13 = { (chunkBlockP.x + 32.0f) / divisor, (chunkBlockP.z + 00.0f) / divisor };

	Vec2 chunk20 = { (chunkBlockP.x - 16.0f) / divisor, (chunkBlockP.z + 16.0f) / divisor };
	Vec2 chunk21 = { (chunkBlockP.x + 00.0f) / divisor, (chunkBlockP.z + 16.0f) / divisor };
	Vec2 chunk22 = { (chunkBlockP.x + 16.0f) / divisor, (chunkBlockP.z + 16.0f) / divisor };
	Vec2 chunk23 = { (chunkBlockP.x + 32.0f) / divisor, (chunkBlockP.z + 16.0f) / divisor };

	Vec2 chunk30 = { (chunkBlockP.x - 16.0f) / divisor, (chunkBlockP.z + 32.0f) / divisor };
	Vec2 chunk31 = { (chunkBlockP.x + 00.0f) / divisor, (chunkBlockP.z + 32.0f) / divisor };
	Vec2 chunk32 = { (chunkBlockP.x + 16.0f) / divisor, (chunkBlockP.z + 32.0f) / divisor };
	Vec2 chunk33 = { (chunkBlockP.x + 32.0f) / divisor, (chunkBlockP.z + 32.0f) / divisor };

    Mat4 c;
	c.e[0] = Noise(chunk00);
	c.e[1] = Noise(chunk10);
	c.e[2] = Noise(chunk20);
	c.e[3] = Noise(chunk30);

	c.e[4] = Noise(chunk01);
	c.e[5] = Noise(chunk11);
	c.e[6] = Noise(chunk21);
	c.e[7] = Noise(chunk31);

	c.e[8] = Noise(chunk02);
	c.e[9] = Noise(chunk12);
	c.e[10] = Noise(chunk22);
	c.e[11] = Noise(chunk32);

	c.e[12] = Noise(chunk03);
	c.e[13] = Noise(chunk13);
	c.e[14] = Noise(chunk23);
	c.e[15] = Noise(chunk33);

    c = c * float(CHUNK_Y);

#endif
#ifdef BILINEAR
	Vec3Int chunkBlockP = BlockPosition();
	const double divisor = static_cast<double>(97);
	Vec2d chunkPbl = { (chunkBlockP.x + 00) / divisor, (chunkBlockP.z + 00) / divisor };
	Vec2d chunkPbr = { (chunkBlockP.x + 16) / divisor, (chunkBlockP.z + 00) / divisor };
	Vec2d chunkPtl = { (chunkBlockP.x + 00) / divisor, (chunkBlockP.z + 16) / divisor };
	Vec2d chunkPtr = { (chunkBlockP.x + 16) / divisor, (chunkBlockP.z + 16) / divisor };
	float bl = Noise(chunkPbl);
	float br = Noise(chunkPbr);
	float tl = Noise(chunkPtl);
	float tr = Noise(chunkPtr);
	bl *= float(CHUNK_Y);
	br *= float(CHUNK_Y);
	tl *= float(CHUNK_Y);
	tr *= float(CHUNK_Y);
	Rect r = {
		.botLeft = { 0,0 },
		.topRight = { 16, 16 },


#endif
	for (int32 x = 0; x < CHUNK_X; x++)
	{
		for (int32 z = 0; z < CHUNK_Z; z++)
		{
			Vec2 blockP = { static_cast<float>(x), static_cast<float>(z) };
#ifdef BILINEAR
			int32 yTotal = static_cast<int32>(Bilinear(blockP,
									r, bl, br, tl, tr));
#endif
#ifdef BICUBIC
			blockP /= 16;
			int32 yTotal = static_cast<int32>(Bicubic(c, blockP));
#endif
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


                        //TODO: fix indices and Vertices ordering
                        //so these checks are not needed
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
	flags &= ~(CHUNK_MODIFIED | CHUNK_LOADING);
	flags |= CHUNK_LOADED;
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
	flags &= ~(CHUNK_NOTUPLOADED);
}
