#include <renderer/storage/material-storage.hpp>

namespace {
SDL_GPUDevice *m_GPU_device;
MaterialStorageType material_storage;

}; // namespace

// Material Storage
namespace MaS {

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
}

void destroy() {}

// Material
handle::Material createMaterial() {
	data::Material default_material{};
	default_material.frag_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.vert.hlsl";
	default_material.vert_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.frag.hlsl";
	return material_storage.insert(default_material);
}
void refMaterial(handle::Material material) {
	material_storage.ref(material);
}
void destroyMaterial(handle::Material material) {
	material_storage.erase(material);
}
data::MaterialOptions getMaterialOptions(handle::Material material) {
	return material_storage.get(material).options;
}
glm::vec4 getMaterialColorFactor(handle::Material material) {
	return material_storage.get(material).factors.color_factor;
}
glm::vec3 getMaterialEmissiveFactor(handle::Material material) {
	return material_storage.get(material).factors.emissive_factor;
}
handle::Texture getMaterialNormalTexture(handle::Material material) {
	return material_storage.get(material).normal;
}
handle::Texture getMaterialEmissiveTexture(handle::Material material) {
	return material_storage.get(material).emissive;
}
handle::Texture getMaterialOcclusionTexture(handle::Material material) {
	return material_storage.get(material).occlusion;
}
handle::Texture getMaterialColorTexture(handle::Material material) {
	return material_storage.get(material).color;
}
handle::Texture getMaterialMetallicRoughnessTexture(handle::Material material) {
	return material_storage.get(material).metallic_roughness;
}
float getMaterialNormalScale(handle::Material material) {
	return material_storage.get(material).factors.normal_scale;
}
float getMaterialMetallicFactor(handle::Material material) {
	return material_storage.get(material).factors.metallic_factor;
}
float getMaterialRoughnessFactor(handle::Material material) {
	return material_storage.get(material).factors.metallic_factor;
}
void setMaterialColorFactor(handle::Material material,
		glm::vec4 color_factor) {
	material_storage.get(material).factors.color_factor = color_factor;
	material_storage.get(material).options |= data::MaterialOptions::COLOR_FACTOR_USED;
	material_storage.setIsEdited(material);
}
void setMaterialEmissiveFactor(handle::Material material,
		glm::vec3 emissive_factor) {
	material_storage.get(material).factors.emissive_factor = emissive_factor;
	material_storage.setIsEdited(material);
}
void setMaterialNormalTexture(handle::Material material,
		handle::Texture normal) {
	TS::refTexture(normal); //! erase previous texture reserve index 0 for
							//! invalid in slotmap
	material_storage.get(material).normal = normal;
	material_storage.get(material).options |= data::MaterialOptions::NORMAL_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialEmissiveTexture(handle::Material material,
		handle::Texture emissive) {
	TS::refTexture(emissive);
	material_storage.get(material).emissive = emissive;
	material_storage.get(material).options |= data::MaterialOptions::EMISSIVE_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialOcclusionTexture(handle::Material material,
		handle::Texture occlusion) {
	TS::refTexture(occlusion);
	material_storage.get(material).occlusion = occlusion;
	material_storage.get(material).options |= data::MaterialOptions::OCCLUSION_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialColorTexture(handle::Material material,
		handle::Texture color) {
	TS::refTexture(color);
	material_storage.get(material).color = color;
	material_storage.get(material).options |= data::MaterialOptions::COLOR_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialMetallicRoughness(handle::Material material,
		handle::Texture metallic_roughness) {
	TS::refTexture(metallic_roughness);
	material_storage.get(material).metallic_roughness = metallic_roughness;
	material_storage.get(material).options |= data::MaterialOptions::METALLIC_ROUGHNESS_TEXTURE;
	material_storage.setIsEdited(material);
}
void setMaterialNormalScale(handle::Material material, float normal_scale) {
	material_storage.get(material).factors.normal_scale = normal_scale;
	material_storage.setIsEdited(material);
}
void setMaterialMetallicFactor(handle::Material material,
		float metallic_factor) {
	material_storage.get(material).factors.metallic_factor = metallic_factor;
	material_storage.setIsEdited(material);
}
void setMaterialRoughnessFactor(handle::Material material,
		float roughness_factor) {
	material_storage.get(material).factors.metallic_factor = roughness_factor;
	material_storage.setIsEdited(material);
}
data::MaterialFactors getMaterialFactors(handle::Material material) {
	return material_storage.get(material).factors;
}
}; // namespace MaS