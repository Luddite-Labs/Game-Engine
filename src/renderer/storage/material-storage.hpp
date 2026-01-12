#pragma once

#include "SDL3/SDL_gpu.h"
#include "misc/slot-map.hpp"
#include <renderer/storage/texture-storage.hpp>
#include <renderer/types.hpp>

using MaterialStorageType = SlotMap<std::vector<data::Material>, data::Material>;

// Material Storage
namespace MaS {

void init(SDL_GPUDevice *device);
void destroy();
handle::Material createMaterial();
void refMaterial(handle::Material material);
void destroyMaterial(handle::Material material);
data::MaterialOptions getMaterialOptions(handle::Material material);
data::MaterialFactors getMaterialFactors(handle::Material material);
glm::vec4 getMaterialColorFactor(handle::Material material);
glm::vec3 getMaterialEmissiveFactor(handle::Material material);
handle::Texture getMaterialNormalTexture(handle::Material material);
handle::Texture getMaterialEmissiveTexture(handle::Material material);
handle::Texture getMaterialOcclusionTexture(handle::Material material);
handle::Texture getMaterialColorTexture(handle::Material material);
handle::Texture getMaterialMetallicRoughnessTexture(handle::Material material);
float getMaterialNormalScale(handle::Material material);
float getMaterialMetallicFactor(handle::Material material);
float getMaterialRoughnessFactor(handle::Material material);
void setMaterialColorFactor(handle::Material material,
		glm::vec4 color_factor);
void setMaterialEmissiveFactor(handle::Material material,
		glm::vec3 emissive_factor);
void setMaterialNormalTexture(handle::Material material,
		handle::Texture normal);
void setMaterialEmissiveTexture(handle::Material material,
		handle::Texture emissive);
void setMaterialOcclusionTexture(handle::Material material,
		handle::Texture occlusion);
void setMaterialColorTexture(handle::Material material,
		handle::Texture color);
void setMaterialMetallicRoughness(handle::Material material,
		handle::Texture metallic_roughness);
void setMaterialNormalScale(handle::Material material, float normal_scale);
void setMaterialMetallicFactor(handle::Material material,
		float metallic_factor);
void setMaterialRoughnessFactor(handle::Material material,
		float roughness_factor);
}; // namespace MS