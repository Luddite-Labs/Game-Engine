#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <cstdint>
#include <cstring>
#include <renderer/types.hpp>

using MeshStorageType = SlotMap<std::vector<RE::Mesh::Data>, RE::Mesh::Data, RE::Mesh::Handle>;

// Mesh Storage
namespace MS {

void init(SDL_GPUDevice *device);
void destroy();

RE::Mesh::Handle
createMesh(RE::Mesh::Arg &mesh_data);
void refMesh(RE::Mesh::Handle mesh);
void destroyMesh(RE::Mesh::Handle mesh);
RE::Mesh::AABB getMeshAABB(RE::Mesh::Handle mesh);
const RE::Mesh::Primitive::Data &getPrimitiveData(RE::Mesh::Handle mesh, uint32_t primitive_index);
void setMeshAABB(RE::Mesh::Handle mesh, RE::Mesh::AABB aabb);
const std::vector<RE::Mesh::Primitive::Data> &getMeshPrimitives(RE::Mesh::Handle mesh);
bool isValid(RE::Mesh::Handle mesh);
}; // namespace MS