#pragma once

#include "glm/trigonometric.hpp"
#include "misc/slot-map.hpp"
#include <glm/common.hpp>
#include <renderer/types.hpp>

using BufferStorageType = SlotMap<std::vector<RE::Buffer::Data>, RE::Buffer::Data, RE::Buffer::Handle>;

// camera storage
namespace BS {
// Camera
void init(SDL_GPUDevice* device);
void destroy();

RE::Buffer::Handle createBuffer(RE::Buffer::Usage usage, uint32_t size);
void refBuffer(RE::Buffer::Handle buffer);
void destroyBuffer(RE::Buffer::Handle buffer);
uint32_t getBufferSize(RE::Buffer::Handle buffer);
RE::Buffer::Usage getBufferUsage(RE::Buffer::Handle buffer);
SDL_GPUBuffer *getBufferGPUHandle(RE::Buffer::Handle buffer);
bool isValid(RE::Buffer::Handle buffer);
}; // namespace CS