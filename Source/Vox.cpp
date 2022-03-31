#include "Vox.h"
#include "WinInterop.h"
#include "Math.h"
#include "Chunk.h"

#include "SDL.h"
#define OGT_VOX_IMPLEMENTATION
#include "OGT_Voxel/ogt_vox.h"
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include "OGT_Voxel/ogt_voxel_meshify.h"

#include <unordered_map>

template <typename T>
T GetDataAndIncriment(const uint8* data, int64& dataIndex)
{
    T result = *((T*)(&(data[dataIndex])));
    dataIndex += sizeof(T);
    return result;
}

std::string GetStringDataAndIncriment(const uint8* data, int64& dataIndex)
{
    int32 stringBufferSize = GetDataAndIncriment<int32>(data, dataIndex);
    std::string s;
    for (int32 c = 0; c < stringBufferSize; c++)
    {
        s += GetDataAndIncriment<char>(data, dataIndex);
    }
    return s;
}

void ReadDictData(std::unordered_map<std::string, std::string>& dict, const uint8* data, int64& dataIndex)
{
    int32 dictSize = GetDataAndIncriment<int32>(data, dataIndex);
    for (int32 i = 0; i < dictSize; i++)
    {
        std::string key   = GetStringDataAndIncriment(data, dataIndex);
        std::string value = GetStringDataAndIncriment(data, dataIndex);
        assert(dict.find(key) == dict.end());
        dict[key] = value;
    }
}

void FourCCToString(std::string& result, uint32 FCC)
{
    result = "    ";
    uint8* data = (uint8*)&FCC;
    for (int32 i = 0; i < sizeof(uint32); i++)
    {
        result[i] = (char)data[i];
    }
}

#pragma pack(push, 1)
struct VoxData
{
    uint8 x;
    uint8 y;
    uint8 z;
    uint8 colorIndex;
};
struct ChunkHeader
{
    uint32 FCCID;
    int32 numChunks;
    int32 numChildren;

    std::string FCCAsString()
    {
        std::string mainHeaderString;
        FourCCToString(mainHeaderString, FCCID);
        return mainHeaderString;
    }

};
#pragma pack(pop)
struct MaterialProperties
{
    std::string _type; //_diffuse, _metal, _glass, _emit
    float _weight;
    float _rough;
    float _spec;
    float _ior;
    float _att;
    float _flux;
    std::string _plastic; //is this in use?
};
#define VOXELS_PER_BLOCK 16
struct VoxelBlockData {
    uint8 e[VOXELS_PER_BLOCK][VOXELS_PER_BLOCK][VOXELS_PER_BLOCK] = {};
};


bool LoadVoxFile_MyImplimentation(const std::string& filePath)
{
    assert(sizeof(VoxData) == 4);
    const uint32 FCCVox  = SDL_FOURCC('V', 'O', 'X', ' ');
    const uint32 FCCMain = SDL_FOURCC('M', 'A', 'I', 'N');
    const uint32 FCCPack = SDL_FOURCC('P', 'A', 'C', 'K');
    const uint32 FCCSize = SDL_FOURCC('S', 'I', 'Z', 'E');
    const uint32 FCCXYZI = SDL_FOURCC('X', 'Y', 'Z', 'I');
    const uint32 FCCnTRN = SDL_FOURCC('n', 'T', 'R', 'N');
    const uint32 FCCnGRP = SDL_FOURCC('n', 'G', 'R', 'P');
    const uint32 FCCnSHP = SDL_FOURCC('n', 'S', 'H', 'P');
    const uint32 FCCMATL = SDL_FOURCC('M', 'A', 'T', 'L');
    const uint32 FCCLAYR = SDL_FOURCC('L', 'A', 'Y', 'R');
    const uint32 FCCrOBJ = SDL_FOURCC('r', 'O', 'B', 'J');
    const uint32 FCCrCAM = SDL_FOURCC('r', 'C', 'A', 'M');
    const uint32 FCCNOTE = SDL_FOURCC('N', 'O', 'T', 'E');
    const uint32 FCCIMAP = SDL_FOURCC('I', 'M', 'A', 'P');

    File file(filePath, File::Mode::Read, false);
    if (!file.m_handleIsValid)
    {
        assert(false);
        return false;
    }

    file.GetData();
    if (!file.m_binaryDataIsValid)
    {
        assert(false);
        return false;
    }

    const uint8* data = file.m_dataBinary.data();
    int64 dataIndex = 0;
    int32 firstFCC = GetDataAndIncriment<int32>(data, dataIndex);
    if (FCCVox != firstFCC)
    {
        assert(false);
        return false;
    }

#if 1
    int32 versionNumber = GetDataAndIncriment<int32>(data, dataIndex);
    int32 sizex = 0;
    int32 sizey = 0;
    int32 sizez = 0;
    std::vector<VoxelBlockData> colorIndices;
    int32 modelIndex = 0;
    std::unordered_map<std::string, std::string> mainDict;
    std::vector<std::unordered_map<std::string, std::string>> frameDicts;
    std::vector<std::unordered_map<std::string, std::string>> modelDicts;
    std::unordered_map<int32, MaterialProperties> materials;
    while (data)
    {
        ChunkHeader header = GetDataAndIncriment<ChunkHeader>(data, dataIndex);
        std::string headerString = header.FCCAsString();

        // process the chunk.
        switch (header.FCCID)
        {
        case FCCMain:
            break;
        case FCCPack:
        {
            int32 numOfModels = GetDataAndIncriment<int32>(data, dataIndex); //dont need
            break;
        }
        case FCCSize:
        {
            assert(header.numChunks == 12 && header.numChildren == 0);
            sizex = GetDataAndIncriment<int32>(data, dataIndex);
            sizey = GetDataAndIncriment<int32>(data, dataIndex);
            sizez = GetDataAndIncriment<int32>(data, dataIndex);
            assert(sizex <= VOXELS_PER_BLOCK);
            assert(sizey <= VOXELS_PER_BLOCK);
            assert(sizez <= VOXELS_PER_BLOCK);
            break;
        }
        case FCCXYZI:
        {
            int32 numVoxels = GetDataAndIncriment<int32>(data, dataIndex);
            colorIndices.push_back({});
            VoxData vox = {};
            for (int32 i = 0; i < numVoxels; i++)
            {
                vox = GetDataAndIncriment<VoxData>(data, dataIndex);
                colorIndices[modelIndex].e[vox.x][vox.y][vox.z] = vox.colorIndex;
            }
            break;
        }
        case FCCnTRN:
        {
            int32 nodeID        = GetDataAndIncriment<int32>(data, dataIndex);
            ReadDictData(mainDict, data, dataIndex);
            int32 childNodeID   = GetDataAndIncriment<int32>(data, dataIndex);
            int32 reservedID    = GetDataAndIncriment<int32>(data, dataIndex);
            assert(reservedID == -1);
            int32 layerID       = GetDataAndIncriment<int32>(data, dataIndex);
            int32 numOfFrames   = GetDataAndIncriment<int32>(data, dataIndex);
            assert(numOfFrames > 0);
            for (int32 i = 0; i < numOfFrames; i++)
            {
                frameDicts.push_back({});
                ReadDictData(frameDicts[i], data, dataIndex);
            }
            break;
        }
        case FCCnGRP:
        {
            int32 childNodeID   = GetDataAndIncriment<int32>(data, dataIndex);
            ReadDictData(mainDict, data, dataIndex);
            int32 numChildNodes = GetDataAndIncriment<int32>(data, dataIndex);
            for (int32 i = 0; i < numChildNodes; i++)
            {
                int32 childNodeID = GetDataAndIncriment<int32>(data, dataIndex);
                //dont need this?
            }

            break;
        }
        case FCCnSHP:
        {
            int32 nodeID = GetDataAndIncriment<int32>(data, dataIndex);
            ReadDictData(mainDict, data, dataIndex);
            int32 numChildNodes = GetDataAndIncriment<int32>(data, dataIndex);
            for (int32 i = 0; i < numChildNodes; i++)
            {
                int32 childNodeID = GetDataAndIncriment<int32>(data, dataIndex);
                modelDicts.push_back({});
                ReadDictData(modelDicts[i], data, dataIndex);
                //dont need this?
            }
            break;
        }
        case FCCMATL:
        {
            int32 materialID = GetDataAndIncriment<int32>(data, dataIndex);
            std::unordered_map<std::string, std::string> d;
            ReadDictData(d, data, dataIndex);
            MaterialProperties m;
            if (d.find("_type") != d.end())
                m._type = d["_type"];
            
            break;
        }
        case FCCLAYR:
        case FCCrOBJ:
        case FCCrCAM:
        case FCCNOTE:
        case FCCIMAP:
        default:
        {
            std::string headerString = header.FCCAsString();
            assert(false);
            break;
        }
        }
    }

    int32 testResult = 123;


#else
    data += sizeof(uint32);
    int32 versionNumber = *(int32*)data;
    data += sizeof(int32);
    uint32 chunkID = (*(uint32*)data);
    data += sizeof(uint32);
    const int32 chunkByteCount =
#endif


    return false;
}

bool CheckForVoxel(Voxels voxels, Vec3Int loc)
{
    if (loc.x >= VOXEL_MAX_SIZE || loc.y >= VOXEL_MAX_SIZE || loc.z >= VOXEL_MAX_SIZE)
        return false;
    return (voxels.e[loc.x][loc.y][loc.z].pack != 0);
}

uint16 VoxelPositionToIndex(Vec3 p)
{
    const uint16 result = (int16(p.z) * 16 * 16) + (int16(p.y) * 16) + (int16(p.x));
    return result;
}

Vec3Int IndexToVoxelPosition(uint16 i)
{
    Vec3Int result;
    result.x = (i) % VOXEL_MAX_SIZE;
    result.y = ((i - result.x) % (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE)) / VOXEL_MAX_SIZE;
    result.z = (i - result.x - (result.y * VOXEL_MAX_SIZE)) / (VOXEL_MAX_SIZE * VOXEL_MAX_SIZE);
    return result;
}

#if 1
void* OGTAlloc(size_t size, void* user_data)
{
    return malloc(size);
}
void OGTFree(void* ptr, void* user_data)
{
    free(ptr);
}

Vec4 GetUnscaledPos(Vec4 p, const Mat4& rot, const Mat4& tran)
{
    p = rot * p;
    p = tran * p;
    return p;
}


bool OGTImplimentation(VoxelMesh& voxelMeshs, std::vector<Voxels>& blocksVoxels, const std::string& filePath)
{
    assert(voxelMeshs.sizes.size() == 0);
    assert(voxelMeshs.vertices.size() == 0);
    assert(voxelMeshs.indices.size() == 0);
    voxelMeshs = {};

    File file(filePath, File::Mode::Read, false);
    if (!file.m_handleIsValid)
    {
        assert(false);
        return false;
    }
    file.GetData();
    if (!file.m_binaryDataIsValid)
    {
        assert(false);
        return false;
    }

    const ogt_vox_scene* scene = ogt_vox_read_scene(file.m_dataBinary.data(), (uint32_t)file.m_dataBinary.size());
    Defer{ ogt_vox_destroy_scene(scene); };
    voxelMeshs.vertices.reserve(scene->num_models);

    //uint32_t                num_models;     // number of models within the scene.
    //uint32_t                num_instances;  // number of instances in the scene
    //uint32_t                num_layers;     // number of layers in the scene
    //uint32_t                num_groups;     // number of groups in the scene
    //const ogt_vox_model**   models;         // array of models. size is num_models
    //const ogt_vox_instance* instances;      // array of instances. size is num_instances
    //const ogt_vox_layer*    layers;         // array of layers. size is num_layers
    //const ogt_vox_group*    groups;         // array of groups. size is num_groups
    //ogt_vox_palette         palette;        // the palette for this scene
    //ogt_vox_matl_array      materials;      // the extended materials for this scene


    if (!scene->num_models)
        return false;
    assert(scene->num_instances == 1);
    ogt_voxel_meshify_context context;
    context.alloc_func = &OGTAlloc;
    context.free_func = &OGTFree;
    ogt_mesh* ogtMesh = nullptr;
    for (uint32 m = 0; m < scene->num_models; m++)
    {
        voxelMeshs.sizes.push_back({});
        voxelMeshs.vertices.push_back({});
        voxelMeshs.indices.push_back({});
        blocksVoxels.push_back({});
        const ogt_vox_model* ogtM = scene->models[m];
        {
            assert(ogtM->size_x <= VOXEL_MAX_SIZE);
            assert(ogtM->size_y <= VOXEL_MAX_SIZE);
            assert(ogtM->size_z <= VOXEL_MAX_SIZE);
            for (uint32 x = 0; x < ogtM->size_x; x++)
            {
                for (uint32 y = 0; y < ogtM->size_y; y++)
                {
                    for (uint32 z = 0; z < ogtM->size_z; z++)
                    {
                        const auto index = ogtM->voxel_data[(z * ogtM->size_x * ogtM->size_y) + (y * ogtM->size_x) + x];
                        assert(index < 256);
                        blocksVoxels[m].e[x][y][z].r = scene->palette.color[index].r;
                        blocksVoxels[m].e[x][y][z].g = scene->palette.color[index].g;
                        blocksVoxels[m].e[x][y][z].b = scene->palette.color[index].b;
                        blocksVoxels[m].e[x][y][z].a = scene->palette.color[index].a;
                    }
                }
            }
        }
        ogtMesh = ogt_mesh_from_paletted_voxels_greedy(&context, ogtM->voxel_data, ogtM->size_x, ogtM->size_y, ogtM->size_z, (ogt_mesh_rgba*)scene->palette.color);
        voxelMeshs.sizes[m].x = ogtM->size_x;
        voxelMeshs.sizes[m].y = ogtM->size_y;
        voxelMeshs.sizes[m].z = ogtM->size_z;

        Mat4 rot;
        gb_mat4_rotate(&rot, { 1, 0, 0 }, tau / 4.0f * 3.0f);
        Mat4 tran;
        gb_mat4_translate(&tran, { 0, 0, 16 }); //WARNING: This is hardcoded and needs to be based off of "something" TODO: Fix
        Mat4 mov = tran * rot;
        uint32 vertIndex = 0;
        Vec3 compoundPos = {};
        for (uint32 i = 0; i < ogtMesh->vertex_count; i++)
        {
            Vec4 oldPos = { ogtMesh->vertices[i].pos.x, ogtMesh->vertices[i].pos.y, ogtMesh->vertices[i].pos.z, 1.0f };
#if 1
            Vec4 newPos = { oldPos.x, oldPos.z, VOXEL_MAX_SIZE - oldPos.y };
#else
            Vec4 newPos = GetUnscaledPos(oldPos, rot, tran);
#endif
            Vec4 nonScaledPos = newPos;
            newPos = newPos / 16.0f;
            Vertex_Voxel v;
            v.p = newPos.xyz;
            v.rgba = {};
            v.rgba.r = ogtMesh->vertices[i].color.r;
            v.rgba.g = ogtMesh->vertices[i].color.g;
            v.rgba.b = ogtMesh->vertices[i].color.b;
            v.rgba.a = ogtMesh->vertices[i].color.a;

            Vec4 ogtNormal = { ogtMesh->vertices[i].normal.x, ogtMesh->vertices[i].normal.y, ogtMesh->vertices[i].normal.z, 1.0f };
            Vec4 newNormal = { ogtNormal.x, ogtNormal.z, -ogtNormal.y, };
            //ogtNormal = rot * ogtNormal;

            if (newNormal.x)
            {
                if (newNormal.x > 0)
                    v.n = +Face::Right;
                else
                    v.n = +Face::Left;
            }
            else if (newNormal.y)
            {
                if (newNormal.y > 0)
                    v.n = +Face::Top;
                else
                    v.n = +Face::Bot;
            }
            else if (newNormal.z)
            {
                if (newNormal.z > 0)
                    v.n = +Face::Back;
                else
                    v.n = +Face::Front;
            }
            else
                assert(false);

#if 0
            voxelMeshs.vertices[m].push_back(v);
            compoundPos += nonScaledPos.xyz;
            vertIndex++;
            if ((vertIndex % 4) == 0)
            {
                vertIndex = 0;
                Vec3 center = compoundPos / 4.0f;
                compoundPos = {};
                for (uint32 j = i; (i - j) < 4; j--)
                {
                    assert(i == voxelMeshs.vertices[m].size() - 1);
                    Vec4 original = { ogtMesh->vertices[j].pos.x, ogtMesh->vertices[j].pos.y, ogtMesh->vertices[j].pos.z, 1.0f };
                    Vec4 unscaledOriginal = GetUnscaledPos(original, rot, tran);
                    Vec3Int destDiff    = Vec3ToVec3Int(Round(unscaledOriginal.xyz - center));
                    destDiff.x = Clamp(destDiff.x, -1, 1);
                    destDiff.y = Clamp(destDiff.y, -1, 1);
                    destDiff.z = Clamp(destDiff.z, -1, 1);
                    Vec3Int voxPos      = Vec3ToVec3Int(Round(original.xyz));
                    Vec3Int normal      = Vec3ToVec3Int(faceNormals[v.n]);
                    //one of these is useless depending on the face but its simpler to check for all of them:
                    voxelMeshs.vertices[m][j].ao = 0;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.x  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.y  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff.z  + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                    if (CheckForVoxel(blocksVoxels[m], voxPos + destDiff    + normal))
                        voxelMeshs.vertices[m][j].ao += 1;
                }
            }
#else
            {
                Vec3Int blockN = Vec3ToVec3Int(faceNormals[v.n]);
                Vec3Int a = *(&vertexBlocksToCheck[v.n].e0 + (vertIndex + 0));
                Vec3Int b = *(&vertexBlocksToCheck[v.n].e0 + (vertIndex + 1));
                Vec3Int c = a + b;
                Vec3Int p = Vec3ToVec3Int(nonScaledPos.xyz + 0.5);
                v.ao = {};
                if (CheckForVoxel(blocksVoxels[m], p + a))
                    v.ao += 1;
                if (CheckForVoxel(blocksVoxels[m], p + b))
                    v.ao += 1;
                if (CheckForVoxel(blocksVoxels[m], p + c))
                    v.ao += 1;

                vertIndex += 2;
                vertIndex = vertIndex % 8;
            }
            voxelMeshs.vertices[m].push_back(v);
#endif
        }
        for (uint32 i = 0; i < ogtMesh->index_count; i++)
        {
            voxelMeshs.indices[m].push_back(ogtMesh->indices[i]);
        }
        ogt_mesh_destroy(&context, ogtMesh);
        ogtMesh = nullptr;
    }

    return true;
}
#endif

bool LoadVoxFile(VoxelMesh& voxelMeshs, std::vector<Voxels>& blocksVoxels, const std::string& filePath)
{
#if 1
    return OGTImplimentation(voxelMeshs, blocksVoxels, filePath);
#else
    return LoadVoxFile_MyImplimentation(filePath);
#endif
}
