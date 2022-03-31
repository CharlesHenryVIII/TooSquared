#pragma once
#include "math.h"

#include <string>
#include <vector>

struct Vertex_Voxel;
struct VoxelMesh {
    std::vector<Vec3Int> sizes;
    std::vector<std::vector<Vertex_Voxel>> vertices;
    std::vector<std::vector<uint32>> indices;
};
#define VOXEL_MAX_SIZE 16
struct Voxels {
    Uint32Pack e[VOXEL_MAX_SIZE][VOXEL_MAX_SIZE][VOXEL_MAX_SIZE] = {};
};

bool LoadVoxFile(VoxelMesh& voxelMeshs, std::vector<Voxels>& blocksVoxels, const std::string& filePath);
