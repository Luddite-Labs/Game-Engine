#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <cstdint>
#include <cstring>
#include <renderer/types.hpp>

using MeshStorageType = SlotMap<std::vector<data::Mesh>, data::Mesh>;

// Mesh Storage
namespace MS {

void init(SDL_GPUDevice *device);
void destroy();

handle::Mesh
createMesh(data::MeshData &mesh_data);

void refMesh(handle::Mesh mesh);
void destroyMesh(handle::Mesh mesh);

AABB getMeshAABB(handle::Mesh mesh);
const data::Primitive &getPrimitiveData(handle::Mesh mesh, uint32_t primitive_index);
void setMeshAABB(handle::Mesh mesh, AABB aabb);
const std::vector<data::Primitive>& getMeshPrimitives(handle::Mesh mesh);
}; // namespace MS