#pragma once
#include "SDL3/SDL_gpu.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include <SDL3/SDL.h>

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <renderer/types.hpp>

#include <misc/slot-map.hpp>
#include <misc/utils.hpp>

// Rendering engine - core renderer class and storage wrappers
namespace RE {
enum class CommandType {
	Mesh,
	UI
};

struct DrawCommand {
	CommandType type;
	glm::mat4x4 transform;
	Mesh::Handle mesh;
};

glm::mat4x4 getProjectionMatrix(const RE::Camera::Handle &camera);

// lifecycle
void init();
void destroy();

// device / render
SDL_GPUDevice *getGPUDevice();
void drawToTexture(const Options &renderer_options,
		const Texture::Handle &target_texture,
		const Camera::Handle &camera,
		const glm::mat4x4 &camera_transform,
		std::vector<DrawCommand> &draw_commands);
uintptr_t getTexture(Texture::Handle texture);

namespace Camera {
Camera::Handle create();
void ref(Camera::Handle camera);
void destroy(Camera::Handle camera);
void setPerspective(Camera::Handle camera, float aspect_ratio,
		float fov = glm::radians(75.0f),
		float near_plane = 1.0f,
		float far_plane = 1000.0f);
void setOrthogonal(Camera::Handle camera, float xmag = 1.0f,
		float ymag = 1.0f, float near_plane = 1.0f,
		float far_plane = 1000.0f);
float getAspectRatio(Camera::Handle camera);
float getFOV(Camera::Handle camera);
float getXMag(Camera::Handle camera);
float getYMag(Camera::Handle camera);
float getNearPlane(Camera::Handle camera);
float getFarPlane(Camera::Handle camera);
bool isOrthogonal(Camera::Handle camera);
void setAspectRatio(Camera::Handle camera, float aspect_ratio);
void setFOV(Camera::Handle camera, float fov);
void setXMag(Camera::Handle camera, float xmag);
void setYMag(Camera::Handle camera, float ymag);
void setNearPlane(Camera::Handle camera, float near_plane);
void setFarPlane(Camera::Handle camera, float far_plane);
void setIsOrthogonal(Camera::Handle camera, bool is_orthogonal);
bool isValid(Camera::Handle camera);
}; // namespace Camera

// Material storage wrappers (CS)
namespace Material {
Material::Handle create();
void ref(Material::Handle material);
void destroy(Material::Handle material);
Material::Options getOptions(Material::Handle material);
glm::vec4 getColorFactor(Material::Handle material);
glm::vec3 getEmissiveFactor(Material::Handle material);
Texture::Handle getNormalTexture(Material::Handle material);
Texture::Handle getEmissiveTexture(Material::Handle material);
Texture::Handle getOcclusionTexture(Material::Handle material);
Texture::Handle getColorTexture(Material::Handle material);
Texture::Handle getMetallicRoughnessTexture(Material::Handle material);
float getNormalScale(Material::Handle material);
float getMetallicFactor(Material::Handle material);
float getRoughnessFactor(Material::Handle material);
float getAlphaCutoff(RE::Material::Handle material);
RE::Material::AlphaModes getAlphaMode(RE::Material::Handle material);
bool getDoubleSided(RE::Material::Handle material);
void setColorFactor(Material::Handle material, glm::vec4 color_factor);
void setEmissiveFactor(Material::Handle material, glm::vec3 emissive_factor);
void setNormalTexture(Material::Handle material, Texture::Handle normal);
void setEmissiveTexture(Material::Handle material, Texture::Handle emissive);
void setOcclusionTexture(Material::Handle material, Texture::Handle occlusion);
void setColorTexture(Material::Handle material, Texture::Handle color);
void setMetallicRoughnessTexture(Material::Handle material, Texture::Handle metallic_roughness);
void setNormalScale(Material::Handle material, float normal_scale);
void setMetallicFactor(Material::Handle material, float metallic_factor);
void setRoughnessFactor(Material::Handle material, float roughness_factor);
void setAlphaCutoff(RE::Material::Handle material, float alpha_cutoff);
void setAlphaMode(RE::Material::Handle material, RE::Material::AlphaModes alpha_mode);
void setDoubleSided(RE::Material::Handle material, bool double_sided);
bool isValid(Material::Handle material);
}; // namespace Material

// Mesh storage wrappers (MS)
namespace Mesh {
Mesh::Handle create(RE::Mesh::Arg &mesh_data);
void ref(Mesh::Handle mesh);
void destroy(Mesh::Handle mesh);
AABB getAABB(Mesh::Handle mesh);
const Primitive::Data &getPrimitiveData(Mesh::Handle mesh,
		uint32_t primitive_index);
void setAABB(Mesh::Handle mesh, AABB aabb);
const std::vector<Mesh::Primitive::Data> &getPrimitives(Mesh::Handle mesh);
bool isValid(Mesh::Handle mesh);
}; // namespace Mesh

// Sampler storage wrappers (SaS)
namespace Sampler {
Sampler::Handle create(
		Sampler::FilteringModes mag_filter = RE::Sampler::FilteringModes::LINEAR,
		Sampler::FilteringModes min_filter = RE::Sampler::FilteringModes::LINEAR,
		Sampler::AddressingModes u_addressing = RE::Sampler::AddressingModes::REPEAT,
		Sampler::AddressingModes v_addressing = RE::Sampler::AddressingModes::REPEAT,
		Sampler::AddressingModes w_addressing = RE::Sampler::AddressingModes::REPEAT,
		Sampler::MipMapMode mip_map_mode = RE::Sampler::MipMapMode::NEAREST,
		bool enable_anisotropy = true);
void ref(Sampler::Handle sampler);
void destroy(Sampler::Handle sampler);
Sampler::FilteringModes getMagFilter(Sampler::Handle sampler);
Sampler::FilteringModes getMinFilter(Sampler::Handle sampler);
Sampler::AddressingModes getUAddressing(Sampler::Handle sampler);
Sampler::AddressingModes getVAddressing(Sampler::Handle sampler);
Sampler::AddressingModes getWAddressing(Sampler::Handle sampler);
Sampler::MipMapMode getMipMapMode(Sampler::Handle sampler);
void setMagFilter(Sampler::Handle sampler,
		Sampler::FilteringModes mode);
void setMinFilter(Sampler::Handle sampler,
		Sampler::FilteringModes mode);
void setUAddressing(Sampler::Handle sampler,
		Sampler::AddressingModes mode);
void setVAddressing(Sampler::Handle sampler,
		Sampler::AddressingModes mode);
bool isValid(Sampler::Handle sampler);
}; // namespace Sampler

// Shader storage wrappers (ShS)
namespace Shader {
Shader::Handle create(const std::string &shader_file,
		const std::vector<Shader::Definition>
				&defines);
void ref(Shader::Handle shader);
void destroy(Shader::Handle shader);
Shader::Type getType(Shader::Handle shader);
bool isValid(Shader::Handle shader);
}; // namespace Shader

// Texture storage wrappers (TS)
namespace Texture {
Texture::Handle create(uint32_t width, uint32_t height,
		Texture::UsageFlags usage_flags = Texture::UsageFlags::SAMPLER,
		Texture::Format format = Texture::Format::R8G8B8A8_UNORM,
		RE::Texture::SampleCount sample_count = RE::Texture::SampleCount::ONE,
		bool generate_mip_maps = false);
void ref(Texture::Handle texture);
void destroy(Texture::Handle texture);
uint32_t getWidth(Texture::Handle texture);
uint32_t getHeight(Texture::Handle texture);
Texture::Format getFormat(Texture::Handle texture);
uint32_t getUsageFlags(Texture::Handle texture);
Sampler::Handle getSampler(Texture::Handle texture);
void setWidth(Texture::Handle texture, uint32_t width);
void setHeight(Texture::Handle texture, uint32_t height);
void setFormat(Texture::Handle texture, Texture::Format format);
void setUsageFlags(Texture::Handle texture, uint32_t usage_flags);
void setSampler(Texture::Handle texture, Sampler::Handle sampler);
void uploadBuffer(Texture::Handle texture,
		std::shared_ptr<uint8_t> buffer, size_t offset,
		size_t count);
bool isValid(Texture::Handle texture);
}; // namespace Texture

namespace Light {
// Light
void init();
void destroy();
void setPosition(Light::Handle light, const glm::vec3 &position);
void setColor(Light::Handle light, const glm::vec3 &color);
glm::vec3 getPosition(Light::Handle light);
glm::vec3 getColor(Light::Handle light);
Light::Handle create();
void ref(Light::Handle light);
void destroy(Light::Handle light);
bool isValid(Light::Handle light);
}; // namespace Light
}; // namespace RE
