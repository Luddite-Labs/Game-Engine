#pragma once

#include "misc/slot-map.hpp"
#include "renderer/types.hpp"

using SamplerStorageType = SlotMap<std::vector<data::Sampler>, data::Sampler>;

namespace SaS {

void init(SDL_GPUDevice *device);

void destroy();

handle::Sampler createSampler(SamplerFilteringModes mag_filter,
		SamplerFilteringModes min_filter,
		SamplerAddressingModes u_addressing,
		SamplerAddressingModes v_addressing);
void refSampler(handle::Sampler sampler);
void destroySampler(handle::Sampler sampler);
SamplerFilteringModes getSamplerMagFilter(handle::Sampler sampler);
SamplerFilteringModes getSamplerMinFilter(handle::Sampler sampler);
SamplerAddressingModes getSamplerUAddressing(handle::Sampler sampler);
SamplerAddressingModes getSamplerVAddressing(handle::Sampler sampler);
void setSamplerMagFilter(handle::Sampler sampler,
		SamplerFilteringModes mode);
void setSamplerMinFilter(handle::Sampler sampler,
		SamplerFilteringModes mode);
void setSamplerUAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode);
void setSamplerVAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode);
SDL_GPUSampler *getSamplerGPUHandle(handle::Sampler sampler);
}; // namespace SaS