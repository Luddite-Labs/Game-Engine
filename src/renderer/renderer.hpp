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
#include <soa_vector.hpp>

// Rendering engine - core renderer class and storage wrappers
namespace RE {
struct DrawCommand {
	glm::mat4x4 transform;
	handle::Mesh mesh;
};

// lifecycle
void init();
void destroy();

// device / render
SDL_GPUDevice *getGPUDevice();
void drawToTexture(const RendererOptions &renderer_options,
		const handle::Texture &target_texture,
		const handle::Camera &camera,
		const glm::mat4x4 &camera_transform,
		std::vector<DrawCommand> &draw_commands);
uintptr_t getTexture(handle::Texture texture);

// Camera storage wrappers (CS)
handle::Camera createCamera();
void refCamera(handle::Camera camera);
void destroyCamera(handle::Camera camera);
void setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
		float fov = glm::radians(75.0f),
		float near_plane = 1.0f,
		float far_plane = 1000.0f);
void setOrthogonalCamera(handle::Camera camera, float xmag = 1.0f,
		float ymag = 1.0f, float near_plane = 1.0f,
		float far_plane = 1000.0f);
float getCameraAspectRatio(handle::Camera camera);
float getCameraFOV(handle::Camera camera);
float getCameraXMag(handle::Camera camera);
float getCameraYMag(handle::Camera camera);
float getCameraNearPlane(handle::Camera camera);
float getCameraFarPlane(handle::Camera camera);
bool isCameraOrthogonal(handle::Camera camera);
void setCameraAspectRatio(handle::Camera camera, float aspect_ratio);
void setCameraFOV(handle::Camera camera, float fov);
void setCameraXMag(handle::Camera camera, float xmag);
void setCameraYMag(handle::Camera camera, float ymag);
void setCameraNearPlane(handle::Camera camera, float near_plane);
void setCameraFarPlane(handle::Camera camera, float far_plane);
void setCameraIsOrthogonal(handle::Camera camera, bool is_orthogonal);

// Material storage wrappers (CS)
handle::Material createMaterial();
void refMaterial(handle::Material material);
void destroyMaterial(handle::Material material);
data::MaterialOptions getMaterialOptions(handle::Material material);
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

// Mesh storage wrappers (MS)
handle::Mesh createMesh(data::MeshData &mesh_data);
void refMesh(handle::Mesh mesh);
void destroyMesh(handle::Mesh mesh);
AABB getMeshAABB(handle::Mesh mesh);
const data::Primitive &getPrimitiveData(handle::Mesh mesh,
		uint32_t primitive_index);
void setMeshAABB(handle::Mesh mesh, AABB aabb);
const std::vector<data::Primitive> &getMeshPrimitives(handle::Mesh mesh);

// Pipeline storage wrappers (PS)
void initPipelineStorage(SDL_GPUDevice *device);
void destroyPipelineStorage();
handle::Pipeline createPipeline(const data::PipelineOptions &options);

// Sampler storage wrappers (SaS)
handle::Sampler createSampler(SamplerFilteringModes mag_filter,
		SamplerFilteringModes min_filter,
		SamplerAddressingModes u_addressing,
		SamplerAddressingModes v_addressing);
void refSampler(handle::Sampler sampler);
void destroySampler(handle::Sampler sampler);
SamplerFilteringModes getSamplerMagFilter(handle::Sampler sampler);
SamplerFilteringModes getSamplerMinFilter(handle::Sampler sampler);
SamplerAddressingModes getSamplerUAddressing(handle::Sampler sampler);
SamplerAddressingModes getSamplerVAddressing(handle::Sampler sampler);
void setSamplerMagFilter(handle::Sampler sampler,
		SamplerFilteringModes mode);
void setSamplerMinFilter(handle::Sampler sampler,
		SamplerFilteringModes mode);
void setSamplerUAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode);
void setSamplerVAddressing(handle::Sampler sampler,
		SamplerAddressingModes mode);

// Shader storage wrappers (ShS)
handle::Shader createShader(const std::string &shader_file,
		const std::vector<data::ShaderDefinition>
				&defines);
void refShader(handle::Shader shader);
void destroyShader(handle::Shader shader);
ShaderType getShaderType(handle::Shader shader);
uint32_t getShaderNumSamplers(handle::Shader shader);
uint32_t getShaderNumStorageTextures(handle::Shader shader);
uint32_t getShaderNumStorageBuffers(handle::Shader shader);
uint32_t getShaderNumUniformBuffers(handle::Shader shader);
SDL_GPUShader *getShaderGPUHandle(handle::Shader shader);

// Texture storage wrappers (TS)
handle::Texture createTexture(uint32_t width, uint32_t height,
		TextureUsageFlags usage_flags,
		TextureFormat format);
void refTexture(handle::Texture texture);
void destroyTexture(handle::Texture texture);
uint32_t getTextureWidth(handle::Texture texture);
uint32_t getTextureHeight(handle::Texture texture);
TextureFormat getTextureFormat(handle::Texture texture);
uint32_t getTextureUsageFlags(handle::Texture texture);
handle::Sampler getTextureSampler(handle::Texture texture);
void setTextureWidth(handle::Texture texture, uint32_t width);
void setTextureHeight(handle::Texture texture, uint32_t height);
void setTextureFormat(handle::Texture texture, TextureFormat format);
void setTextureUsageFlags(handle::Texture texture, uint32_t usage_flags);
void setTextureSampler(handle::Texture texture, handle::Sampler sampler);
void uploadBufferToTexture(handle::Texture texture,
		std::shared_ptr<uint8_t> buffer, size_t offset,
		size_t count);

}; // namespace RE