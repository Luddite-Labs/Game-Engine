#include "glm/gtc/type_ptr.hpp"
#include <imgui.h>
#include <renderer/storage/material-storage.hpp>
namespace {
SDL_GPUDevice *m_GPU_device;
MaterialStorageType material_storage;

}; // namespace

// Material Storage
namespace MaS {

void drawMaterialDebugUI(RE::Material::Data &material_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&material_data));
	if (ImGui::CollapsingHeader(("Material - " + std::to_string(reinterpret_cast<size_t>(&material_data))).c_str())) {
		float metallic_factor = material_data.factors.metallic_factor;
		float roughness_factor = material_data.factors.roughness_factor;
		float normal_scale = material_data.factors.normal_scale;
		float alpha_cutoff = material_data.factors.alpha_cutoff;
		glm::vec4 color_factor = material_data.factors.color_factor;
		glm::vec3 emissive_factor = material_data.factors.emissive_factor;
		bool double_sided = material_data.factors.double_sided;
		if (ImGui::InputFloat("Metallic Factor", &metallic_factor)) {
			material_data.factors.metallic_factor = metallic_factor;
		}
		if (ImGui::InputFloat("Roughness Factor", &roughness_factor)) {
			material_data.factors.roughness_factor = roughness_factor;
		}
		if (ImGui::InputFloat("Normal Scale", &normal_scale)) {
			material_data.factors.normal_scale = normal_scale;
		}
		if (ImGui::InputFloat4("Color Factor", glm::value_ptr(color_factor))) {
			material_data.factors.color_factor = color_factor;
		}
		if (ImGui::InputFloat3("Emissive Factor", glm::value_ptr(emissive_factor))) {
			material_data.factors.emissive_factor = emissive_factor;
		}
		if (ImGui::BeginCombo("Alpha Modes", getString(material_data.factors.alpha_mode))) {
			if (ImGui::Selectable(getString(RE::Material::AlphaModes::OPAQUE), material_data.factors.alpha_mode == RE::Material::AlphaModes::OPAQUE)) {
				material_data.factors.alpha_mode = RE::Material::AlphaModes::OPAQUE;
			}
			if (ImGui::Selectable(getString(RE::Material::AlphaModes::MASK), material_data.factors.alpha_mode == RE::Material::AlphaModes::MASK)) {
				material_data.factors.alpha_mode = RE::Material::AlphaModes::MASK;
			}
			if (ImGui::Selectable(getString(RE::Material::AlphaModes::BLEND), material_data.factors.alpha_mode == RE::Material::AlphaModes::BLEND)) {
				material_data.factors.alpha_mode = RE::Material::AlphaModes::BLEND;
			}
			ImGui::EndCombo();
		}
		if (ImGui::InputFloat("Alpha Cutoff", &alpha_cutoff)) {
			material_data.factors.alpha_cutoff = alpha_cutoff;
		}
		ImGui::BeginDisabled();
		if (ImGui::Checkbox("Double Sided", &double_sided)) {
			material_data.factors.double_sided = double_sided;
		}
		ImGui::EndDisabled();
		if (TS::isValid(material_data.color)) {
			ImGui::Text("Color Texture");
			ImGui::Indent();
			TS::drawTextureDebugUI(material_data.color);
			ImGui::Unindent();
		}
		if (TS::isValid(material_data.emissive)) {
			ImGui::Text("Emissive Texture");
			ImGui::Indent();
			TS::drawTextureDebugUI(material_data.emissive);
			ImGui::Unindent();
		}
		if (TS::isValid(material_data.normal)) {
			ImGui::Text("Normal Texture");
			ImGui::Indent();
			TS::drawTextureDebugUI(material_data.normal);
			ImGui::Unindent();
		}
		if (TS::isValid(material_data.occlusion)) {
			ImGui::Text("Occlusion Texture");
			ImGui::Indent();
			TS::drawTextureDebugUI(material_data.occlusion);
			ImGui::Unindent();
		}
		if (TS::isValid(material_data.metallic_roughness)) {
			ImGui::Text("Metallic Roughness Texture");
			ImGui::Indent();
			TS::drawTextureDebugUI(material_data.metallic_roughness);
			ImGui::Unindent();
		}
	}
	ImGui::PopID();
}
void drawMaterialDebugUI(RE::Material::Handle &material_handle) {
	drawMaterialDebugUI(material_storage.get(material_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	registerUIDebugCallback("material-storage", [&]() {
		ImGui::TextUnformatted(("Material count:" + std::to_string(material_storage.size())).c_str());
		for (int i = 0; i < material_storage.size(); i++) {
			drawMaterialDebugUI(material_storage[i]);
		}
	});
}

void destroy() {}

// Material
RE::Material::Handle createMaterial() {
	RE::Material::Data default_material{};
	default_material.frag_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.vert.slang";
	default_material.vert_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.frag.slang";
	return material_storage.insert(default_material);
}
void refMaterial(RE::Material::Handle material) {
	material_storage.ref(material);
}
void destroyMaterial(RE::Material::Handle material) {
	material_storage.erase(material);
}
RE::Material::Options getMaterialOptions(RE::Material::Handle material) {
	return material_storage.get(material).options;
}
glm::vec4 getMaterialColorFactor(RE::Material::Handle material) {
	return material_storage.get(material).factors.color_factor;
}
glm::vec3 getMaterialEmissiveFactor(RE::Material::Handle material) {
	return material_storage.get(material).factors.emissive_factor;
}
RE::Texture::Handle getMaterialNormalTexture(RE::Material::Handle material) {
	return material_storage.get(material).normal;
}
RE::Texture::Handle getMaterialEmissiveTexture(RE::Material::Handle material) {
	return material_storage.get(material).emissive;
}
RE::Texture::Handle getMaterialOcclusionTexture(RE::Material::Handle material) {
	return material_storage.get(material).occlusion;
}
RE::Texture::Handle getMaterialColorTexture(RE::Material::Handle material) {
	return material_storage.get(material).color;
}
RE::Texture::Handle getMaterialMetallicRoughnessTexture(RE::Material::Handle material) {
	return material_storage.get(material).metallic_roughness;
}
float getMaterialNormalScale(RE::Material::Handle material) {
	return material_storage.get(material).factors.normal_scale;
}
float getMaterialMetallicFactor(RE::Material::Handle material) {
	return material_storage.get(material).factors.metallic_factor;
}
float getMaterialRoughnessFactor(RE::Material::Handle material) {
	return material_storage.get(material).factors.roughness_factor;
}
bool getDoubleSided(RE::Material::Handle material) {
	return material_storage.get(material).factors.double_sided;
}
void setMaterialColorFactor(RE::Material::Handle material,
		glm::vec4 color_factor) {
	material_storage.get(material).factors.color_factor = color_factor;
	material_storage.get(material).options |= RE::Material::Options::COLOR_FACTOR_USED;
	material_storage.setIsEdited(material);
}
void setMaterialEmissiveFactor(RE::Material::Handle material,
		glm::vec3 emissive_factor) {
	material_storage.get(material).factors.emissive_factor = emissive_factor;
	material_storage.setIsEdited(material);
}
void setMaterialNormalTexture(RE::Material::Handle material,
		RE::Texture::Handle normal) {
	TS::refTexture(normal); //! erase previous texture reserve index 0 for
							//! invalid in slotmap
	material_storage.get(material).normal = normal;
	material_storage.get(material).options |= RE::Material::Options::NORMAL_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialEmissiveTexture(RE::Material::Handle material,
		RE::Texture::Handle emissive) {
	TS::refTexture(emissive);
	material_storage.get(material).emissive = emissive;
	material_storage.get(material).options |= RE::Material::Options::EMISSIVE_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialOcclusionTexture(RE::Material::Handle material,
		RE::Texture::Handle occlusion) {
	TS::refTexture(occlusion);
	material_storage.get(material).occlusion = occlusion;
	material_storage.get(material).options |= RE::Material::Options::OCCLUSION_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialColorTexture(RE::Material::Handle material,
		RE::Texture::Handle color) {
	TS::refTexture(color);
	material_storage.get(material).color = color;
	material_storage.get(material).options |= RE::Material::Options::COLOR_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialMetallicRoughness(RE::Material::Handle material,
		RE::Texture::Handle metallic_roughness) {
	TS::refTexture(metallic_roughness);
	material_storage.get(material).metallic_roughness = metallic_roughness;
	material_storage.get(material).options |= RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialNormalScale(RE::Material::Handle material, float normal_scale) {
	material_storage.get(material).factors.normal_scale = normal_scale;
	material_storage.setIsEdited(material);
}
void setMaterialMetallicFactor(RE::Material::Handle material,
		float metallic_factor) {
	material_storage.get(material).factors.metallic_factor = metallic_factor;
	material_storage.setIsEdited(material);
}
void setMaterialRoughnessFactor(RE::Material::Handle material,
		float roughness_factor) {
	material_storage.get(material).factors.roughness_factor = roughness_factor;
	material_storage.setIsEdited(material);
}
RE::Material::Factors getMaterialFactors(RE::Material::Handle material) {
	return material_storage.get(material).factors;
}
bool isValid(RE::Material::Handle material) {
	return material_storage.isValid(material);
}

float getMaterialAlphaCutoff(RE::Material::Handle material) {
	return material_storage.get(material).factors.alpha_cutoff;
}

RE::Material::AlphaModes getMaterialAlphaMode(RE::Material::Handle material) {
	return material_storage.get(material).factors.alpha_mode;
}

void setMaterialAlphaCutoff(RE::Material::Handle material, float alpha_cutoff) {
	material_storage.get(material).factors.alpha_cutoff = alpha_cutoff;
	material_storage.setIsEdited(material);
}

void setDoubleSided(RE::Material::Handle material, bool double_sided) {
	material_storage.get(material).factors.double_sided = double_sided;
	material_storage.setIsEdited(material);
}
void setMaterialAlphaMode(RE::Material::Handle material, RE::Material::AlphaModes alpha_mode) {
	material_storage.get(material).factors.alpha_mode = alpha_mode;
	material_storage.setIsEdited(material);
}

}; // namespace MaS