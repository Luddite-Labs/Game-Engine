#include "SDL3/SDL_assert.h"
#include <renderer/storage/sampler-storage.hpp>

using SamplerStorageType = SlotMap<std::vector<RE::Sampler::Data>, RE::Sampler::Data, RE::Sampler::Handle>;

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

RE::Sampler::Handle createSampler(RE::Sampler::FilteringModes mag_filter,
		RE::Sampler::FilteringModes min_filter,
		RE::Sampler::AddressingModes u_addressing,
		RE::Sampler::AddressingModes v_addressing,
		RE::Sampler::AddressingModes w_addressing,
		RE::Sampler::MipMapMode mip_map_mode) {
	SDL_assert(m_GPU_device != nullptr);
	RE::Sampler::Data sampler_data = {
		.gpu_handle = nullptr,
		.mag_filter = mag_filter,
		.min_filter = min_filter,
		.u_addressing = u_addressing,
		.v_addressing = v_addressing,
		.w_addressing = w_addressing,
		.mip_map_mode = mip_map_mode
	};
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = static_cast<SDL_GPUFilter>(sampler_data.min_filter),
		.mag_filter = static_cast<SDL_GPUFilter>(sampler_data.mag_filter),
		.mipmap_mode = static_cast<SDL_GPUSamplerMipmapMode>(sampler_data.mip_map_mode),
		.address_mode_u =
				static_cast<SDL_GPUSamplerAddressMode>(sampler_data.u_addressing),
		.address_mode_v =
				static_cast<SDL_GPUSamplerAddressMode>(sampler_data.v_addressing),
		.address_mode_w =
				static_cast<SDL_GPUSamplerAddressMode>(sampler_data.w_addressing),
		.max_anisotropy = 8.0f,
		.max_lod = 1000.f,
		.enable_anisotropy = true
	};
	sampler_data.gpu_handle = SDL_CreateGPUSampler(m_GPU_device, &sampler_info);
	return sampler_storage.insert(sampler_data);
}
void refSampler(RE::Sampler::Handle sampler) {
	sampler_storage.ref(sampler);
}
void destroySampler(RE::Sampler::Handle sampler) {
	sampler_storage.erase(sampler);
}
RE::Sampler::FilteringModes getSamplerMagFilter(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).mag_filter;
}
RE::Sampler::FilteringModes getSamplerMinFilter(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).min_filter;
}
RE::Sampler::AddressingModes getSamplerUAddressing(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).u_addressing;
}
RE::Sampler::AddressingModes getSamplerVAddressing(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).v_addressing;
}
RE::Sampler::AddressingModes getSamplerWAddressing(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).w_addressing;
}
RE::Sampler::MipMapMode getSamplerMipMapMode(RE::Sampler::Handle sampler) {
	return sampler_storage.get(sampler).mip_map_mode;
}
void setSamplerMagFilter(RE::Sampler::Handle sampler,
		RE::Sampler::FilteringModes mode) {
	sampler_storage.get(sampler).mag_filter = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerMinFilter(RE::Sampler::Handle sampler,
		RE::Sampler::FilteringModes mode) {
	sampler_storage.get(sampler).min_filter = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerUAddressing(RE::Sampler::Handle sampler,
		RE::Sampler::AddressingModes mode) {
	sampler_storage.get(sampler).u_addressing = mode;
	sampler_storage.setIsEdited(sampler);
}
void setSamplerVAddressing(RE::Sampler::Handle sampler,
		RE::Sampler::AddressingModes mode) {
	sampler_storage.get(sampler).v_addressing = mode;
	sampler_storage.setIsEdited(sampler);
}
SDL_GPUSampler *getSamplerGPUHandle(RE::Sampler::Handle sampler) {
	SDL_assert(sampler_storage.isValid(sampler));
	return sampler_storage.get(sampler).gpu_handle;
}
bool isValid(RE::Sampler::Handle sampler) {
	return sampler_storage.isValid(sampler);
}
}; // namespace SaS