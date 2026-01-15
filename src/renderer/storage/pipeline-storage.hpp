#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <renderer/types.hpp>

using PipelineStorageType = SlotMap<std::vector<RE::Pipeline::Data>, RE::Pipeline::Data, RE::Pipeline::Handle>;

// Pipeline Storage
namespace PS {
void init(SDL_GPUDevice *device);
void destroy();
RE::Pipeline::Handle createPipeline(const RE::Pipeline::Options &options);
SDL_GPUGraphicsPipeline* getPipelineGPUHandle(RE::Pipeline::Handle pipeline);
bool isValid(RE::Pipeline::Handle pipeline);
}; // namespace PS