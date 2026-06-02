#include "SDL3/SDL_assert.h"
#include <imgui.h>
#include <renderer/storage/sampler-storage.hpp>

using SamplerStorageType = SlotMap<std::vector<RE::Sampler::Data>, RE::Sampler::Data, RE::Sampler::Handle>;

namespace {
SDL_GPUDevice *m_GPU_device;
SamplerStorageType sampler_storage;
}; // namespace

namespace SaS {
void drawSamplerDebugUI(RE::Sampler::Data &sampler_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&sampler_data));
	if (ImGui::CollapsingHeader(("Sampler - " + std::to_string(reinterpret_cast<size_t>(&sampler_data))).c_str())) {
		bool edit = false;
		RE::Sampler::FilteringModes mag_filter = sampler_data.mag_filter;
		RE::Sampler::FilteringModes min_filter = sampler_data.min_filter;
		RE::Sampler::AddressingModes u_addressing = sampler_data.u_addressing;
		RE::Sampler::AddressingModes v_addressing = sampler_data.v_addressing;
		RE::Sampler::AddressingModes w_addressing = sampler_data.w_addressing;
		RE::Sampler::MipMapMode mip_map_mode = sampler_data.mip_map_mode;
		bool enable_anisotropy = sampler_data.enable_anisotropy;
		if (ImGui::BeginCombo("Mag Filter", getString(mag_filter))) {
			if (ImGui::Selectable(getString(RE::Sampler::FilteringModes::LINEAR), mag_filter == RE::Sampler::FilteringModes::LINEAR)) {
				edit = true;
				mag_filter = RE::Sampler::FilteringModes::LINEAR;
			}
			if (mag_filter == RE::Sampler::FilteringModes::LINEAR) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::FilteringModes::NEAREST), mag_filter == RE::Sampler::FilteringModes::NEAREST)) {
				edit = true;
				mag_filter = RE::Sampler::FilteringModes::NEAREST;
			}
			if (mag_filter == RE::Sampler::FilteringModes::NEAREST) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImGui::BeginCombo("Min Filter", getString(min_filter))) {
			if (ImGui::Selectable(getString(RE::Sampler::FilteringModes::LINEAR), min_filter == RE::Sampler::FilteringModes::LINEAR)) {
				edit = true;
				min_filter = RE::Sampler::FilteringModes::LINEAR;
			}
			if (min_filter == RE::Sampler::FilteringModes::LINEAR) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::FilteringModes::NEAREST), min_filter == RE::Sampler::FilteringModes::NEAREST)) {
				edit = true;
				min_filter = RE::Sampler::FilteringModes::NEAREST;
			}
			if (min_filter == RE::Sampler::FilteringModes::NEAREST) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImGui::BeginCombo("U Addressing", getString(u_addressing))) {
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::REPEAT), u_addressing == RE::Sampler::AddressingModes::REPEAT)) {
				edit = true;
				u_addressing = RE::Sampler::AddressingModes::REPEAT;
			}
			if (u_addressing == RE::Sampler::AddressingModes::REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::CLAMP_TO_EDGE), u_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE)) {
				edit = true;
				u_addressing = RE::Sampler::AddressingModes::CLAMP_TO_EDGE;
			}
			if (u_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::MIRRORED_REPEAT), u_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT)) {
				edit = true;
				u_addressing = RE::Sampler::AddressingModes::MIRRORED_REPEAT;
			}
			if (u_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImGui::BeginCombo("V Addressing", getString(v_addressing))) {
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::REPEAT), v_addressing == RE::Sampler::AddressingModes::REPEAT)) {
				edit = true;
				v_addressing = RE::Sampler::AddressingModes::REPEAT;
			}
			if (v_addressing == RE::Sampler::AddressingModes::REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::CLAMP_TO_EDGE), v_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE)) {
				edit = true;
				v_addressing = RE::Sampler::AddressingModes::CLAMP_TO_EDGE;
			}
			if (v_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::MIRRORED_REPEAT), v_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT)) {
				edit = true;
				v_addressing = RE::Sampler::AddressingModes::MIRRORED_REPEAT;
			}
			if (v_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImGui::BeginCombo("W Addressing", getString(w_addressing))) {
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::REPEAT), w_addressing == RE::Sampler::AddressingModes::REPEAT)) {
				edit = true;
				w_addressing = RE::Sampler::AddressingModes::REPEAT;
			}
			if (w_addressing == RE::Sampler::AddressingModes::REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::CLAMP_TO_EDGE), w_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE)) {
				edit = true;
				w_addressing = RE::Sampler::AddressingModes::CLAMP_TO_EDGE;
			}
			if (w_addressing == RE::Sampler::AddressingModes::CLAMP_TO_EDGE) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::AddressingModes::MIRRORED_REPEAT), w_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT)) {
				edit = true;
				w_addressing = RE::Sampler::AddressingModes::MIRRORED_REPEAT;
			}
			if (w_addressing == RE::Sampler::AddressingModes::MIRRORED_REPEAT) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImGui::BeginCombo("Mip Map Mode", getString(mip_map_mode))) {
			if (ImGui::Selectable(getString(RE::Sampler::MipMapMode::LINEAR), mip_map_mode == RE::Sampler::MipMapMode::LINEAR)) {
				edit = true;
				mip_map_mode = RE::Sampler::MipMapMode::LINEAR;
			}
			if (mip_map_mode == RE::Sampler::MipMapMode::LINEAR) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(getString(RE::Sampler::MipMapMode::NEAREST), mip_map_mode == RE::Sampler::MipMapMode::NEAREST)) {
				edit = true;
				mip_map_mode = RE::Sampler::MipMapMode::NEAREST;
			}
			if (mip_map_mode == RE::Sampler::MipMapMode::NEAREST) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (edit) {
			LOG_INFO("Updating Sampler");
			sampler_data.mag_filter = mag_filter;
			sampler_data.min_filter = min_filter;
			sampler_data.u_addressing = u_addressing;
			sampler_data.v_addressing = v_addressing;
			sampler_data.w_addressing = w_addressing;
			sampler_data.mip_map_mode = mip_map_mode;
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
				.enable_anisotropy = enable_anisotropy
			};
			SDL_ReleaseGPUSampler(m_GPU_device, sampler_data.gpu_handle);
			sampler_data.gpu_handle = SDL_CreateGPUSampler(m_GPU_device, &sampler_info);
		}
	}
	ImGui::PopID();
}
void drawSamplerDebugUI(RE::Sampler::Handle &sampler_handle) {
	drawSamplerDebugUI(sampler_storage.get(sampler_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	registerUIDebugCallback("sampler-storage", [&]() {
		ImGui::TextUnformatted(("Sampler count:" + std::to_string(sampler_storage.size())).c_str());
		for (int i = 0; i < sampler_storage.size(); i++) {
			drawSamplerDebugUI(sampler_storage[i]);
		}
	});
}

void destroy() {
}

RE::Sampler::Handle createSampler(RE::Sampler::FilteringModes mag_filter,
		RE::Sampler::FilteringModes min_filter,
		RE::Sampler::AddressingModes u_addressing,
		RE::Sampler::AddressingModes v_addressing,
		RE::Sampler::AddressingModes w_addressing,
		RE::Sampler::MipMapMode mip_map_mode,
		bool enable_anisotropy) {
	SDL_assert(m_GPU_device != nullptr);
	RE::Sampler::Data sampler_data = {
		.gpu_handle = nullptr,
		.mag_filter = mag_filter,
		.min_filter = min_filter,
		.u_addressing = u_addressing,
		.v_addressing = v_addressing,
		.w_addressing = w_addressing,
		.mip_map_mode = mip_map_mode,
		.enable_anisotropy = enable_anisotropy
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
		.enable_anisotropy = enable_anisotropy
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