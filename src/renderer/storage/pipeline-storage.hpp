#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <renderer/types.hpp>

using PipelineStorageType = SlotMap<std::vector<data::Pipeline>, data::Pipeline>;

// Pipeline Storage
namespace PS {
void init(SDL_GPUDevice *device);
void destroy();
handle::Pipeline createPipeline(const data::PipelineOptions &options);
SDL_GPUGraphicsPipeline* getPipelineGPUHandle(handle::Pipeline pipeline);
}; // namespace PS