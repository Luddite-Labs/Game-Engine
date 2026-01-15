#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <renderer/storage/texture-storage.hpp>
#include <renderer/types.hpp>

using MaterialStorageType = SlotMap<std::vector<RE::Material::Data>, RE::Material::Data, RE::Material::Handle>;

// Material Storage
namespace MaS {

void init(SDL_GPUDevice *device);
void destroy();
RE::Material::Handle createMaterial();
void refMaterial(RE::Material::Handle material);
void destroyMaterial(RE::Material::Handle material);
RE::Material::Options getMaterialOptions(RE::Material::Handle material);
RE::Material::Factors getMaterialFactors(RE::Material::Handle material);
glm::vec4 getMaterialColorFactor(RE::Material::Handle material);
glm::vec3 getMaterialEmissiveFactor(RE::Material::Handle material);
RE::Texture::Handle getMaterialNormalTexture(RE::Material::Handle material);
RE::Texture::Handle getMaterialEmissiveTexture(RE::Material::Handle material);
RE::Texture::Handle getMaterialOcclusionTexture(RE::Material::Handle material);
RE::Texture::Handle getMaterialColorTexture(RE::Material::Handle material);
RE::Texture::Handle getMaterialMetallicRoughnessTexture(RE::Material::Handle material);
float getMaterialNormalScale(RE::Material::Handle material);
float getMaterialMetallicFactor(RE::Material::Handle material);
float getMaterialRoughnessFactor(RE::Material::Handle material);
void setMaterialColorFactor(RE::Material::Handle material,
		glm::vec4 color_factor);
void setMaterialEmissiveFactor(RE::Material::Handle material,
		glm::vec3 emissive_factor);
void setMaterialNormalTexture(RE::Material::Handle material,
		RE::Texture::Handle normal);
void setMaterialEmissiveTexture(RE::Material::Handle material,
		RE::Texture::Handle emissive);
void setMaterialOcclusionTexture(RE::Material::Handle material,
		RE::Texture::Handle occlusion);
void setMaterialColorTexture(RE::Material::Handle material,
		RE::Texture::Handle color);
void setMaterialMetallicRoughness(RE::Material::Handle material,
		RE::Texture::Handle metallic_roughness);
void setMaterialNormalScale(RE::Material::Handle material, float normal_scale);
void setMaterialMetallicFactor(RE::Material::Handle material,
		float metallic_factor);
void setMaterialRoughnessFactor(RE::Material::Handle material,
		float roughness_factor);
bool isValid(RE::Material::Handle material);
}; // namespace MaS