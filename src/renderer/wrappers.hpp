#pragma once

#include "SDL3/SDL_gpu.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "misc/slot-map.hpp"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <renderer/renderer.hpp>
#include <variant>

/**
 * Handle based architecture all real code and memory storage in the
 * rendering system and use these classes that contain handles to
 * interface in a normal OOP way to the data
 */
// add copy functions
namespace interface {
class Camera {
private:
	handle::Camera handle;

public:
	Camera() {
		handle = RE::createCamera();
		RE::setOrthogonalCamera(handle);
	}
	Camera(const Camera &other) {
		handle = other.handle;
		RE::refCamera(handle);
	}
	~Camera() { RE::destroyCamera(handle); }
	float getAspectRatio() { return RE::getCameraAspectRatio(handle); }
	float getFOV() { return RE::getCameraFOV(handle); }
	float getXMag() { return RE::getCameraXMag(handle); }
	float getYMag() { return RE::getCameraYMag(handle); }
	float getNearPlane() {
		return RE::getCameraNearPlane(handle);
	}
	float getFarPlane() {
		return RE::getCameraFarPlane(handle);
	}
	void setAspectRatio(float aspect_ratio) {
		RE::setCameraAspectRatio(handle, aspect_ratio);
	}
	void setFOV(float fov) {
		RE::setCameraFOV(handle, fov);
	}
	void setXMag(float xmag) {
		RE::setCameraXMag(handle, xmag);
	}
	void setYMag(float ymag) {
		RE::setCameraYMag(handle, ymag);
	}
	void setNearPlane(float near_plane) {
		RE::setCameraNearPlane(handle, near_plane);
	}
	void setFarPlane(float far_plane) {
		RE::setCameraFarPlane(handle, far_plane);
	}
	bool isOrthogonal() const {
		return RE::isCameraOrthogonal(handle);
	}
	void setIsOrthogonal(bool is_orthogonal) {
		RE::setCameraIsOrthogonal(handle, is_orthogonal);
	}
	void setDefaultPerspective() {
		RE::setPerspectiveCamera(handle, 1.77);
	}
	void setDefaultOrthographic() {
		RE::setOrthogonalCamera(handle);
	}
	friend class Renderer;
};

// Use data::MeshData from renderer/types.hpp

struct Sampler {
private:
	handle::Sampler handle;
	Sampler(handle::Sampler handle) : handle(handle) {}

public:
	Sampler(SamplerFilteringModes mag_filter = SamplerFilteringModes::LINEAR,
			SamplerFilteringModes min_filter = SamplerFilteringModes::LINEAR,
			SamplerAddressingModes u_addressing =
					SamplerAddressingModes::CLAMP_TO_EDGE,
			SamplerAddressingModes v_addressing =
					SamplerAddressingModes::CLAMP_TO_EDGE) {
		handle = RE::createSampler(mag_filter, min_filter, u_addressing, v_addressing);
	}

	Sampler(const Sampler &other) {
		handle = other.handle;
		RE::refSampler(handle);
	}
	~Sampler() { RE::destroySampler(handle); }

	SamplerFilteringModes getMagFilter() {
		return RE::getSamplerMagFilter(handle);
	}
	SamplerFilteringModes getMinFilter() {
		return RE::getSamplerMinFilter(handle);
	}
	SamplerAddressingModes getUAddressing() {
		return RE::getSamplerUAddressing(handle);
	}
	SamplerAddressingModes getVAddressing() {
		return RE::getSamplerVAddressing(handle);
	}
	void setMagFilter(SamplerFilteringModes mode) {
		RE::setSamplerMagFilter(handle, mode);
	}
	void setMinFilter(SamplerFilteringModes mode) {
		RE::setSamplerMinFilter(handle, mode);
	}
	void setUAddressing(SamplerAddressingModes mode) {
		RE::setSamplerUAddressing(handle, mode);
	}
	void setVAddressing(SamplerAddressingModes mode) {
		RE::setSamplerVAddressing(handle, mode);
	}
	friend class Texture;
};

class Texture {
private:
	handle::Texture handle;

	Texture(handle::Texture handle) : handle(handle) {}

public:
	Texture(uint32_t width, uint32_t height,
			TextureUsageFlags usage_flags = TextureUsageFlags::SAMPLER,
			TextureFormat format = TextureFormat::R8G8B8A8_UNORM) {
		handle = RE::createTexture(width, height, usage_flags, format);
	}
	Texture(const Texture &other) {
		handle = other.handle;
		RE::refTexture(handle);
	}
	~Texture() { RE::destroyTexture(handle); }
	uint32_t getWidth() const {
		return RE::getTextureWidth(handle);
	}
	bool isValid() {
		return SlotMap<interface::Texture,
				std::vector<interface::Texture>>::isValid(handle);
	}
	uint32_t getHeight() const {
		return RE::getTextureHeight(handle);
	}
	TextureFormat getFormat() {
		return RE::getTextureFormat(handle);
	}
	uint32_t getUsageFlags() {
		return RE::getTextureUsageFlags(handle);
	}
	interface::Sampler getSampler() {
		handle::Sampler sampler = RE::getTextureSampler(handle);
		RE::refSampler(sampler);
		return { sampler };
	}
	void setWidth(uint32_t width) {
		RE::setTextureWidth(handle, width);
	}
	void setHeight(uint32_t height) {
		RE::setTextureHeight(handle, height);
	}
	void setFormat(TextureFormat format) {
		RE::setTextureFormat(handle, format);
	}
	void setUsageFlags(uint32_t usage_flags) {
		RE::setTextureUsageFlags(handle, usage_flags);
	}
	void uploadBuffer(std::shared_ptr<uint8_t> buffer, size_t offset,
			size_t count) {
		RE::uploadBufferToTexture(handle, buffer, offset, count);
	}
	void setSampler(interface::Sampler sampler) {
		RE::setTextureSampler(handle, sampler.handle);
	}
	friend class Material;
	friend class Renderer;
};

struct Material {
private:
	handle::Material handle;

public:
	Material() { handle = RE::createMaterial(); }
	Material(const Material &other) {
		handle = other.handle;
		RE::refMaterial(handle);
	}
	~Material() { RE::destroyMaterial(handle); }
	glm::vec4 getColorFactor() { return RE::getMaterialColorFactor(handle); }
	glm::vec3 getEmissiveFactor() { return RE::getMaterialEmissiveFactor(handle); }
	interface::Texture getNormalTexture() {
		handle::Texture texture = RE::getMaterialNormalTexture(handle);
		RE::refTexture(texture);
		return { texture };
	}
	interface::Texture getEmissiveTexture() {
		handle::Texture texture = RE::getMaterialEmissiveTexture(handle);
		RE::refTexture(texture);
		return { texture };
	}
	interface::Texture getOcclusionTexture() {
		handle::Texture texture = RE::getMaterialOcclusionTexture(handle);
		RE::refTexture(texture);
		return { texture };
	}
	interface::Texture getColorTexture() {
		handle::Texture texture = RE::getMaterialColorTexture(handle);
		RE::refTexture(texture);
		return { texture };
	}
	interface::Texture getMetallicRoughness() {
		handle::Texture texture = RE::getMaterialMetallicRoughnessTexture(handle);
		RE::refTexture(texture);
		return { texture };
	}
	float getNormalScale() {
		return RE::getMaterialNormalScale(handle);
	}
	float getMetallicFactor() {
		return RE::getMaterialMetallicFactor(handle);
	}
	float getRoughnessFactor() {
		return RE::getMaterialRoughnessFactor(handle);
	}
	void setColorFactor(glm::vec4 color_factor) {
		RE::setMaterialColorFactor(handle, color_factor);
	}
	void setEmissiveFactor(glm::vec3 emissive_factor) {
		RE::setMaterialEmissiveFactor(handle, emissive_factor);
	}
	void setNormalTexture(Texture texture) {
		RE::setMaterialNormalTexture(handle, texture.handle);
	}
	void setEmissiveTexture(Texture texture) {
		RE::setMaterialEmissiveTexture(handle, texture.handle);
	}
	void setOcclusionTexture(Texture texture) {
		RE::setMaterialOcclusionTexture(handle, texture.handle);
	}
	void setColorTexture(Texture texture) {
		RE::setMaterialColorTexture(handle, texture.handle);
	}
	void setMetallicRoughnessTexture(Texture texture) {
		RE::setMaterialMetallicRoughness(handle, texture.handle);
	}
	void setNormalScale(float normal_scale) {
		RE::setMaterialNormalScale(handle, normal_scale);
	}
	void setMetallicFactor(float metallic_factor) {
		RE::setMaterialMetallicFactor(handle, metallic_factor);
	}
	void setRoughnessFactor(float roughness_factor) {
		RE::setMaterialRoughnessFactor(handle, roughness_factor);
	}
	friend class Renderer;
	friend class Mesh;
};

struct PrimitiveData {
	data::PrimitiveType type;
	std::unique_ptr<uint8_t[]> attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::MAX)];
	uint32_t vert_count;
	uint32_t index_count;
	Material material;
};
struct MeshData {
	std::vector<PrimitiveData> primitives;
	AABB aabb;
};

class Mesh {
private:
	handle::Mesh handle;

public:
	Mesh(MeshData &mesh_data) {
		uploadBuffers(mesh_data);
	}

	Mesh(const Mesh &other) {
		handle = other.handle;
		RE::refMesh(handle);
	}
	~Mesh() {
		RE::destroyMesh(handle);
	}

	AABB getAABB() {
		return RE::getMeshAABB(handle);
	}
	uint32_t getVertCount() {
		if (!SlotMap<std::vector<data::Mesh>, data::Mesh>::isValid(handle)) {
			return 0;
		}
		return RE::getPrimitiveData(handle, 0).vert_count;
	}
	uint32_t getIndexCount() {
		if (!SlotMap<std::vector<data::Mesh>, data::Mesh>::isValid(handle)) {
			return 0;
		}
		return RE::getPrimitiveData(handle, 0).index_count;
	}
	void setAABB(AABB aabb) {
		RE::setMeshAABB(handle, aabb);
	}
	void uploadBuffers(MeshData &mesh_data) {
		RE::destroyMesh(handle);
		data::MeshData rd_mesh_data;
		rd_mesh_data.aabb = mesh_data.aabb;
		rd_mesh_data.primitives.resize(mesh_data.primitives.size());
		for (size_t i = 0; i < mesh_data.primitives.size(); i++) {
			rd_mesh_data.primitives[i].type = mesh_data.primitives[i].type;
			rd_mesh_data.primitives[i].vert_count = mesh_data.primitives[i].vert_count;
			rd_mesh_data.primitives[i].index_count = mesh_data.primitives[i].index_count;
			rd_mesh_data.primitives[i].material = mesh_data.primitives[i].material.handle;
			for (size_t j = 0; j < static_cast<size_t>(data::VertAttributeIndex::MAX); j++) {
				rd_mesh_data.primitives[i].attrs_data[j].swap(mesh_data.primitives[i].attrs_data[j]);
			}
		}
		handle = RE::createMesh(rd_mesh_data);
	}
	friend class Renderer;
};

class Shader {
private:
	handle::Shader handle;

public:
	Shader(const char *shader_file_path, uint32_t num_samplers = 0,
			uint32_t num_storage_textures = 0, uint32_t num_storage_buffers = 0,
			uint32_t num_uniform_buffers = 0,
			ShaderType shader_type = ShaderType::VERTEX) {
		handle = RE::createShader(std::string(shader_file_path), {});
	}
	Shader(const Shader &other) {
		handle = other.handle;
		RE::refShader(handle);
	}
	~Shader() { RE::destroyShader(handle); }

	uint32_t getNumSamplers(handle::Shader shader) {
		return RE::getShaderNumSamplers(handle);
	}
	uint32_t getNumStorageTextures(handle::Shader shader) {
		return RE::getShaderNumStorageTextures(handle);
	}
	uint32_t getNumStorageBuffers(handle::Shader shader) {
		return RE::getShaderNumStorageBuffers(handle);
	}
	uint32_t getNumUniformBuffers(handle::Shader shader) {
		return RE::getShaderNumUniformBuffers(handle);
	}
	ShaderType getType(handle::Shader shader) {
		return RE::getShaderType(handle);
	}
};

struct DrawCommand {
public:
	interface::Mesh &mesh;
	glm::mat4x4 transform;

	DrawCommand(interface::Mesh &mesh, glm::mat4x4 transform) : mesh(mesh), transform(transform) {}

	DrawCommand(const DrawCommand &other) : mesh(other.mesh), transform(other.transform) {
	}
};

class Renderer {
public:
	static void init() { RE::init(); }
	static void destroy() { RE::destroy(); }

	static void draw(const RendererOptions &opts, Texture target_texture, const Camera &camera,
			const glm::mat4x4 &camera_transform,
			const std::vector<DrawCommand> _draw_commands) {
		std::vector<RE::DrawCommand> draw_commands;
		for (auto &draw_command : _draw_commands) {
			draw_commands.push_back({
					.transform = draw_command.transform,
					.mesh = draw_command.mesh.handle,
			});
		}
		RE::drawToTexture(opts, target_texture.handle, camera.handle, camera_transform, draw_commands);
	}

	static uintptr_t getTexture(const Texture tex) {
		return RE::getTexture(tex.handle);
	}
};

}; // namespace interface