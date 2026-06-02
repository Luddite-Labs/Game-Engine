#pragma once

#include "misc/slot-map.hpp"
#include "renderer/types.hpp"

using SamplerStorageType = SlotMap<std::vector<RE::Sampler::Data>, RE::Sampler::Data, RE::Sampler::Handle>;

namespace SaS {
void drawSamplerDebugUI(RE::Sampler::Data &sampler_data);
void drawSamplerDebugUI(RE::Sampler::Handle &sampler_handle);
void init(SDL_GPUDevice *device);

void destroy();

RE::Sampler::Handle createSampler(RE::Sampler::FilteringModes mag_filter,
		RE::Sampler::FilteringModes min_filter,
		RE::Sampler::AddressingModes u_addressing,
		RE::Sampler::AddressingModes v_addressing,
		RE::Sampler::AddressingModes w_addressing,
		RE::Sampler::MipMapMode mip_map_mode,
		bool enable_anisotropy = true);
void refSampler(RE::Sampler::Handle sampler);
void destroySampler(RE::Sampler::Handle sampler);
RE::Sampler::FilteringModes getSamplerMagFilter(RE::Sampler::Handle sampler);
RE::Sampler::FilteringModes getSamplerMinFilter(RE::Sampler::Handle sampler);
RE::Sampler::AddressingModes getSamplerUAddressing(RE::Sampler::Handle sampler);
RE::Sampler::AddressingModes getSamplerVAddressing(RE::Sampler::Handle sampler);
RE::Sampler::AddressingModes getSamplerWAddressing(RE::Sampler::Handle sampler);
RE::Sampler::MipMapMode getSamplerMipMapMode(RE::Sampler::Handle sampler);
void setSamplerMagFilter(RE::Sampler::Handle sampler,
		RE::Sampler::FilteringModes mode);
void setSamplerMinFilter(RE::Sampler::Handle sampler,
		RE::Sampler::FilteringModes mode);
void setSamplerUAddressing(RE::Sampler::Handle sampler,
		RE::Sampler::AddressingModes mode);
void setSamplerVAddressing(RE::Sampler::Handle sampler,
		RE::Sampler::AddressingModes mode);
SDL_GPUSampler *getSamplerGPUHandle(RE::Sampler::Handle sampler);
bool isValid(RE::Sampler::Handle sampler);
}; // namespace SaS