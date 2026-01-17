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
RE::Material::Handle createMaterial() {
	RE::Material::Data default_material{};
	default_material.frag_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.vert.hlsl";
	default_material.vert_shader_path = GAME_ENGINE_DEFAULT_SHADER_DIR "base.frag.hlsl";
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
	material_storage.get(material).factors.metallic_factor = roughness_factor;
	material_storage.setIsEdited(material);
}
RE::Material::Factors getMaterialFactors(RE::Material::Handle material) {
	return material_storage.get(material).factors;
}
bool isValid(RE::Material::Handle material){
	return material_storage.isValid(material);
}
}; // namespace MaS