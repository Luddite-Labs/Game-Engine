#include "SDL3/SDL_assert.h"
#include <renderer/storage/sampler-storage.hpp>

using SamplerStorageType = SlotMap<std::vector<data::Sampler>, data::Sampler>;

namespace {
SDL_GPUDevice *m_GPU_device;
SamplerStorageType sampler_storage;
}; // namespace

namespace SaS {

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
}

void destroy() {
}

handle::Sampler createSampler(SamplerFilteringModes mag_filter,
		SamplerFilteringModes min_filter,
		SamplerAddressingModes u_addressing,
		SamplerAddressingModes v_addressing) {
	SDL_assert(m_GPU_device != nullptr);
	data::Sampler sampler_data = { .mag_filter = mag_filter,
		.min_filter = min_filter,
		.u_addressing = u_addressing,
		.v_addressing = v_addressing,
		.gpu_handle = nullptr };
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = static_cast<SDL_GPUFilter>(sampler_data.min_filter),
		.mag_filter = static_cast<SDL_GPUFilter>(sampler_data.mag_filter),
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u =
				static_cast<SDL_GPUSamplerAddressMode>(sampler_data.u_addressing),
		.address_mode_v =
				static_cast<SDL_GPUSamplerAddressMode>(sampler_data.v_addressing),
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};
	sampler_data.gpu_handle = SDL_CreateGPUSampler(m_GPU_device, &sampler_info);
	return sampler_storage.insert(sampler_data);
}
void refSampler(handle::Sampler sampler) {
	sampler_storage.ref(sampler);
}
void destroySampler(handle::Sampler sampler) {
	sampler_storage.erase(sampler);
}
SamplerFilteringModes getSamplerMagFilter(handle::Sampler sampler) {
	return sampler_storage.get(sampler).mag_filter;
}
SamplerFilteringModes getSamplerMinFilter(handle::Sampler sampler) {
	return sampler_storage.get(sampler).min_filter;
}
SamplerAddressingModes getSamplerUAddressing(handle::Sampler sampler) {
	return sampler_storage.get(sampler).u_addressing;
}
SamplerAddressingModes getSamplerVAddressing(handle::Sampler sampler) {
	return sampler_storage.get(sampler).v_addressing;
}
void setSamplerMagFilter(handle::Sampler sampler,
		SamplerFilteringModes mode) {
	sampler_storage.get(sampler).mag_filter = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerMinFilter(handle::Sampler sampler,
		SamplerFilteringModes mode) {
	sampler_storage.get(sampler).min_filter = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerUAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode) {
	sampler_storage.get(sampler).u_addressing = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerVAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode) {
	sampler_storage.get(sampler).v_addressing = mode;
	sampler_storage.setIsEdited(sampler);
}
SDL_GPUSampler *getSamplerGPUHandle(handle::Sampler sampler) {
	SDL_assert(SamplerStorageType::isValid(sampler));
	return sampler_storage.get(sampler).gpu_handle;
}

}; // namespace SaS