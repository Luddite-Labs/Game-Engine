#include <algorithm>
#include <filesystem>
#include <renderer/renderer.hpp>
#include <string>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <renderer/storage/camera-storage.hpp>
#include <renderer/storage/material-storage.hpp>
#include <renderer/storage/mesh-storage.hpp>
#include <renderer/storage/pipeline-storage.hpp>
#include <renderer/storage/sampler-storage.hpp>
#include <renderer/storage/shader-storage.hpp>
#include <renderer/storage/texture-storage.hpp>

#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include "misc/slot-map.hpp"
#include "renderer/types.hpp"
#include "scene/components.hpp"
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <misc/log.hpp>
#include <vector>

namespace {
struct TransformMatrices {
	glm::mat4x4 modelView;
	glm::mat4x4 proj;
};

struct PrimitiveRenderInfo {
	glm::mat4x4 transform;
	const RE::Mesh::Primitive::Data *data;
	RE::Pipeline::Handle pipeline;
	RE::Material::Handle material;
};

struct RendererData {
	RE::Texture::Handle dummy_texture;
	RE::Texture::Handle depth_texture;
	RE::Sampler::Handle dummy_sampler;
};

SDL_Window *m_window = nullptr;
SDL_GPUDevice *m_GPU_device = nullptr;
SDL_GPUGraphicsPipeline *m_mesh_pipeline = nullptr;
RE::Shader::Handle m_default_vert_shader = { 0, 0 };
RE::Shader::Handle m_default_frag_shader = { 0, 0 };
RendererData m_renderer_data;

glm::mat4x4 getProjectionMatrix(const RE::Camera::Handle &camera) {
	if (CS::isCameraOrthogonal(camera)) {
		return glm::ortho(0.0f, CS::getCameraXMag(camera), 0.0f, CS::getCameraYMag(camera), CS::getCameraNearPlane(camera),
				CS::getCameraFarPlane(camera));
	} else {
		return glm::perspective(CS::getCameraFOV(camera), CS::getCameraAspectRatio(camera), CS::getCameraNearPlane(camera),
				CS::getCameraFarPlane(camera));
	}
}

void regenerateDepthTexture(
		uint32_t width,
		uint32_t height) {
	if (TextureStorageType::isValid(m_renderer_data.depth_texture)) {
		TS::destroyTexture(m_renderer_data.depth_texture);
	}
	m_renderer_data.depth_texture = TS::createTexture(width, height,
			RE::Texture::UsageFlags::SAMPLER | RE::Texture::UsageFlags::DEPTH_STENCIL_TARGET,
			RE::Texture::Format::D16_UNORM);
}

bool isMeshInViewFrustum(RE::Mesh::Handle mesh, glm::mat4x4 viewProj) {
	return true;
}
}; // namespace

namespace RE {

void init() {
	m_GPU_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	ShS::init(m_GPU_device);
	TS::init(m_GPU_device);
	SaS::init(m_GPU_device);
	PS::init(m_GPU_device);
	MaS::init(m_GPU_device);
	MS::init(m_GPU_device);
	m_renderer_data.dummy_texture = TS::createTexture(1, 1,
			RE::Texture::UsageFlags::SAMPLER,
			RE::Texture::Format::R8G8B8A8_UNORM);
	TS::uploadBufferToTexture(m_renderer_data.dummy_texture,
			std::shared_ptr<uint8_t>(new uint8_t[4]{ 255, 255, 255, 255 },
					std::default_delete<uint8_t[]>()),
			0, 4);
	m_renderer_data.dummy_sampler = SaS::createSampler(
			RE::Sampler::FilteringModes::NEAREST,
			RE::Sampler::FilteringModes::NEAREST,
			RE::Sampler::AddressingModes::CLAMP_TO_EDGE,
			RE::Sampler::AddressingModes::CLAMP_TO_EDGE,
			RE::Sampler::AddressingModes::CLAMP_TO_EDGE);
	m_renderer_data.depth_texture = TS::createTexture(1, 1,
			RE::Texture::UsageFlags::SAMPLER | RE::Texture::UsageFlags::DEPTH_STENCIL_TARGET,
			RE::Texture::Format::D16_UNORM);
}

void destroy() {
	// destroy storages (reverse order)
	MS::destroy();
	MaS::destroy();
	PS::destroy();
	SaS::destroy();
	TS::destroy();
	ShS::destroy();

	SDL_WaitForGPUIdle(m_GPU_device);
	SDL_ReleaseWindowFromGPUDevice(m_GPU_device, m_window);
	SDL_DestroyGPUDevice(m_GPU_device);
	SDL_DestroyWindow(m_window);
}

SDL_GPUDevice *getGPUDevice() {
	return m_GPU_device;
}

SDL_GPUTextureSamplerBinding getSamplerBinding(
		const RE::Texture::Handle &texture_handle) {
	SDL_GPUTextureSamplerBinding binding = {
		.texture = TS::getTextureGPUHandle(texture_handle),
		.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler)
	};
	RE::Sampler::Handle tex_sampler = TS::getTextureSampler(texture_handle);
	if (SamplerStorageType::isValid(tex_sampler)) {
		binding.sampler = SaS::getSamplerGPUHandle(tex_sampler);
	}
	return binding;
}

//! infinite projection
void drawToTexture(const RE::Options &renderer_options,
		const RE::Texture::Handle &target_texture,
		const RE::Camera::Handle &camera,
		const glm::mat4x4 &camera_transform,
		std::vector<DrawCommand> &draw_commands) {
	if (draw_commands.size() == 0) {
		return;
	}
	TransformMatrices transform_matrices;
	transform_matrices.proj = getProjectionMatrix(camera);
	if (TS::getTextureWidth(target_texture) != TS::getTextureWidth(m_renderer_data.depth_texture) ||
			TS::getTextureHeight(target_texture) != TS::getTextureHeight(m_renderer_data.depth_texture)) {
		regenerateDepthTexture(
				TS::getTextureWidth(target_texture),
				TS::getTextureHeight(target_texture));
		return;
	}

	std::vector<PrimitiveRenderInfo> primitives;

	for (const auto draw_command : draw_commands) {
		if (not isMeshInViewFrustum(draw_command.mesh, transform_matrices.proj * camera_transform)) {
			continue;
		}
		const auto &mesh_primitives = MS::getMeshPrimitives(draw_command.mesh);
		for (const auto &primitive : mesh_primitives) {
			PrimitiveRenderInfo render_info = { .transform = draw_command.transform,
				.data = &primitive,
				.pipeline = primitive.pipeline,
				.material = primitive.material };
			primitives.push_back(render_info);
		}
	}
	sort(primitives.begin(), primitives.end(),
			[&](const auto &lhs, const auto &rhs) {
				if (lhs.data->pipeline.slot_index == rhs.data->pipeline.slot_index) {
					return lhs.material.slot_index < rhs.material.slot_index;
				} else {
					return lhs.data->pipeline.slot_index < rhs.data->pipeline.slot_index;
				}
			});

	SDL_GPUCommandBuffer *command_buffer =
			SDL_AcquireGPUCommandBuffer(m_GPU_device);
	SDL_assert(command_buffer);
	SDL_GPUColorTargetInfo color_target_infos[] = {
		{ .texture = TS::getTextureGPUHandle(target_texture),
				.mip_level = 0,
				.layer_or_depth_plane = 0,
				.clear_color = { .r = renderer_options.clear_color.r,
						.g = renderer_options.clear_color.g,
						.b = renderer_options.clear_color.b,
						.a = renderer_options.clear_color.a },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.resolve_texture = nullptr,
				.resolve_mip_level = 0,
				.resolve_layer = 0,
				.cycle = false,
				.cycle_resolve_texture = false }
	};
	SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
		.texture = TS::getTextureGPUHandle(m_renderer_data.depth_texture),
		.clear_depth = 1,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
		.cycle = true,
		.clear_stencil = 0,
	};
	SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
			command_buffer, color_target_infos, 1, &depth_stencil_target_info);
	for (const auto [transform, primitive, pipeline, material] : primitives) {
		//! group mesh by material
		if (MaterialStorageType::isValid(primitive->material)) {
			const auto material_factors = MaS::getMaterialFactors(primitive->material);
			SDL_PushGPUFragmentUniformData(command_buffer, 0, &material_factors,
					sizeof(RE::Material::Factors));
			std::vector<SDL_GPUTextureSamplerBinding> sampler_bindings;
			sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
					.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
			if (TextureStorageType::isValid(
						MaS::getMaterialColorTexture(primitive->material))) {
				sampler_bindings.back() = getSamplerBinding(
						MaS::getMaterialColorTexture(primitive->material));
			}
			sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
					.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
			if (TextureStorageType::isValid(
						MaS::getMaterialNormalTexture(primitive->material))) {
				sampler_bindings.back() = getSamplerBinding(
						MaS::getMaterialNormalTexture(primitive->material));
			}
			sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
					.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
			if (TextureStorageType::isValid(
						MaS::getMaterialEmissiveTexture(primitive->material))) {
				sampler_bindings.back() = getSamplerBinding(
						MaS::getMaterialEmissiveTexture(primitive->material));
			}
			sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
					.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
			if (TextureStorageType::isValid(
						MaS::getMaterialMetallicRoughnessTexture(primitive->material))) {
				sampler_bindings.back() = getSamplerBinding(
						MaS::getMaterialMetallicRoughnessTexture(primitive->material));
			}
			sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
					.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
			if (TextureStorageType::isValid(
						MaS::getMaterialOcclusionTexture(primitive->material))) {
				sampler_bindings.back() = getSamplerBinding(
						MaS::getMaterialOcclusionTexture(primitive->material));
			}
			SDL_BindGPUFragmentSamplers(render_pass, 0, sampler_bindings.data(),
					sampler_bindings.size());
		}
		transform_matrices.modelView = camera_transform * transform;
		SDL_PushGPUVertexUniformData(command_buffer, 0, &transform_matrices,
				sizeof(transform_matrices));

		bool has_index_data = false;
		for (uint32_t i = 0;
				i < static_cast<uint32_t>(RE::Vertex::AttributeIndex::MAX); i++) {
			if (static_cast<RE::Vertex::AttributeIndex>(i) ==
							RE::Vertex::AttributeIndex::INDEX and
					primitive->attrs_data[i].gpu_buffer != nullptr) {
				has_index_data = true;
				SDL_GPUBufferBinding index_binding = {
					.buffer = primitive->attrs_data[i].gpu_buffer, .offset = 0
				};
				SDL_BindGPUIndexBuffer(render_pass, &index_binding,
						SDL_GPU_INDEXELEMENTSIZE_16BIT);
			} else {
				if (primitive->attrs_data[i].gpu_buffer != nullptr) {
					SDL_GPUBufferBinding binding = { .buffer = primitive->attrs_data[i].gpu_buffer, .offset = 0 };
					SDL_BindGPUVertexBuffers(render_pass, i, &binding,
							1);
				}
			}
		}
		//! nullDescriptor VUlkan vertex bindings in the future
		SDL_BindGPUGraphicsPipeline(
				render_pass, PS::getPipelineGPUHandle(primitive->pipeline));
		if (has_index_data) {
			SDL_DrawGPUIndexedPrimitives(render_pass, primitive->index_count, 1, 0, 0,
					0);
		} else {
			SDL_DrawGPUPrimitives(render_pass, primitive->vert_count, 1, 0, 0);
		}
	}
	SDL_EndGPURenderPass(render_pass);
	SDL_SubmitGPUCommandBuffer(command_buffer);
}

//! deprecate once UI backend ready
uintptr_t getTexture(RE::Texture::Handle texture) {
	return reinterpret_cast<uintptr_t>(TS::getTextureGPUHandle(texture));
}

// Camera wrappers (CS)
namespace Camera {
RE::Camera::Handle create() {
	return CS::createCamera();
}
void ref(RE::Camera::Handle camera) {
	CS::refCamera(camera);
}
void destroy(RE::Camera::Handle camera) {
	CS::destroyCamera(camera);
}
void setPerspective(RE::Camera::Handle camera, float aspect_ratio,
		float fov, float near_plane,
		float far_plane) {
	CS::setPerspectiveCamera(camera, aspect_ratio, fov, near_plane, far_plane);
}
void setOrthogonal(RE::Camera::Handle camera, float xmag,
		float ymag, float near_plane,
		float far_plane) {
	CS::setOrthogonalCamera(camera, xmag, ymag, near_plane, far_plane);
}
float getAspectRatio(RE::Camera::Handle camera) {
	return CS::getCameraAspectRatio(camera);
}
float getFOV(RE::Camera::Handle camera) {
	return CS::getCameraFOV(camera);
}
float getXMag(RE::Camera::Handle camera) {
	return CS::getCameraXMag(camera);
}
float getYMag(RE::Camera::Handle camera) {
	return CS::getCameraYMag(camera);
}
float getNearPlane(RE::Camera::Handle camera) {
	return CS::getCameraNearPlane(camera);
}
float getFarPlane(RE::Camera::Handle camera) {
	return CS::getCameraFarPlane(camera);
}
bool isOrthogonal(RE::Camera::Handle camera) {
	return CS::isCameraOrthogonal(camera);
}
void setAspectRatio(RE::Camera::Handle camera, float aspect_ratio) {
	CS::setCameraAspectRatio(camera, aspect_ratio);
}
void setFOV(RE::Camera::Handle camera, float fov) {
	CS::setCameraFOV(camera, fov);
}
void setXMag(RE::Camera::Handle camera, float xmag) {
	CS::setCameraXMag(camera, xmag);
}
void setYMag(RE::Camera::Handle camera, float ymag) {
	CS::setCameraYMag(camera, ymag);
}
void setNearPlane(RE::Camera::Handle camera, float near_plane) {
	CS::setCameraNearPlane(camera, near_plane);
}
void setFarPlane(RE::Camera::Handle camera, float far_plane) {
	CS::setCameraFarPlane(camera, far_plane);
}
void setIsOrthogonal(RE::Camera::Handle camera, bool is_orthogonal) {
	CS::setCameraIsOrthogonal(camera, is_orthogonal);
}
bool isValid(RE::Camera::Handle camera) {
	return CS::isValid(camera);
}
}; // namespace Camera

// Material wrappers (MaS)
namespace Material {
RE::Material::Handle create() {
	return MaS::createMaterial();
}
void ref(RE::Material::Handle material) {
	MaS::refMaterial(material);
}
void destroy(RE::Material::Handle material) {
	MaS::destroyMaterial(material);
}
RE::Material::Options getOptions(RE::Material::Handle material) {
	return MaS::getMaterialOptions(material);
}
glm::vec4 getColorFactor(RE::Material::Handle material) {
	return MaS::getMaterialColorFactor(material);
}
glm::vec3 getEmissiveFactor(RE::Material::Handle material) {
	return MaS::getMaterialEmissiveFactor(material);
}
RE::Texture::Handle getNormalTexture(RE::Material::Handle material) {
	return MaS::getMaterialNormalTexture(material);
}
RE::Texture::Handle getEmissiveTexture(RE::Material::Handle material) {
	return MaS::getMaterialEmissiveTexture(material);
}
RE::Texture::Handle getOcclusionTexture(RE::Material::Handle material) {
	return MaS::getMaterialOcclusionTexture(material);
}
RE::Texture::Handle getColorTexture(RE::Material::Handle material) {
	return MaS::getMaterialColorTexture(material);
}
RE::Texture::Handle getMetallicRoughnessTexture(RE::Material::Handle material) {
	return MaS::getMaterialMetallicRoughnessTexture(material);
}
float getNormalScale(RE::Material::Handle material) {
	return MaS::getMaterialNormalScale(material);
}
float getMetallicFactor(RE::Material::Handle material) {
	return MaS::getMaterialMetallicFactor(material);
}
float getRoughnessFactor(RE::Material::Handle material) {
	return MaS::getMaterialRoughnessFactor(material);
}
void setColorFactor(RE::Material::Handle material, glm::vec4 color_factor) {
	MaS::setMaterialColorFactor(material, color_factor);
}
void setEmissiveFactor(RE::Material::Handle material, glm::vec3 emissive_factor) {
	MaS::setMaterialEmissiveFactor(material, emissive_factor);
}
void setNormalTexture(RE::Material::Handle material, RE::Texture::Handle normal) {
	MaS::setMaterialNormalTexture(material, normal);
}
void setEmissiveTexture(RE::Material::Handle material, RE::Texture::Handle emissive) {
	MaS::setMaterialEmissiveTexture(material, emissive);
}
void setOcclusionTexture(RE::Material::Handle material, RE::Texture::Handle occlusion) {
	MaS::setMaterialOcclusionTexture(material, occlusion);
}
void setColorTexture(RE::Material::Handle material, RE::Texture::Handle color) {
	MaS::setMaterialColorTexture(material, color);
}
void setMetallicRoughnessTexture(RE::Material::Handle material, RE::Texture::Handle metallic_roughness) {
	MaS::setMaterialMetallicRoughness(material, metallic_roughness);
}
void setNormalScale(RE::Material::Handle material, float normal_scale) {
	MaS::setMaterialNormalScale(material, normal_scale);
}
void setMetallicFactor(RE::Material::Handle material, float metallic_factor) {
	MaS::setMaterialMetallicFactor(material, metallic_factor);
}
void setRoughnessFactor(RE::Material::Handle material, float roughness_factor) {
	MaS::setMaterialRoughnessFactor(material, roughness_factor);
}
bool isValid(RE::Material::Handle material) {
	return MaS::isValid(material);
}
}; // namespace Material

// Mesh wrappers (MS)
namespace Mesh {
RE::Mesh::Handle create(RE::Mesh::Arg &mesh_data) {
	return MS::createMesh(mesh_data);
}
void ref(RE::Mesh::Handle mesh) {
	MS::refMesh(mesh);
}
void destroy(RE::Mesh::Handle mesh) {
	MS::destroyMesh(mesh);
}
AABB getAABB(RE::Mesh::Handle mesh) {
	return MS::getMeshAABB(mesh);
}
const RE::Mesh::Primitive::Data &getPrimitiveData(RE::Mesh::Handle mesh, uint32_t primitive_index) {
	return MS::getPrimitiveData(mesh, primitive_index);
}
void setAABB(RE::Mesh::Handle mesh, AABB aabb) {
	MS::setMeshAABB(mesh, aabb);
}
const std::vector<RE::Mesh::Primitive::Data> &getPrimitives(RE::Mesh::Handle mesh) {
	return MS::getMeshPrimitives(mesh);
}
bool isValid(RE::Mesh::Handle mesh) {
	return MS::isValid(mesh);
}
}; // namespace Mesh

// Sampler wrappers (SaS)
namespace Sampler {
RE::Sampler::Handle create(
		RE::Sampler::FilteringModes mag_filter,
		RE::Sampler::FilteringModes min_filter,
		RE::Sampler::AddressingModes u_addressing,
		RE::Sampler::AddressingModes v_addressing,
		RE::Sampler::AddressingModes w_addressing) {
	return SaS::createSampler(mag_filter, min_filter, u_addressing, v_addressing, w_addressing);
}
void ref(RE::Sampler::Handle sampler) {
	SaS::refSampler(sampler);
}
void destroy(RE::Sampler::Handle sampler) {
	SaS::destroySampler(sampler);
}
RE::Sampler::FilteringModes getMagFilter(RE::Sampler::Handle sampler) {
	return SaS::getSamplerMagFilter(sampler);
}
RE::Sampler::FilteringModes getMinFilter(RE::Sampler::Handle sampler) {
	return SaS::getSamplerMinFilter(sampler);
}
RE::Sampler::AddressingModes getUAddressing(RE::Sampler::Handle sampler) {
	return SaS::getSamplerUAddressing(sampler);
}
RE::Sampler::AddressingModes getVAddressing(RE::Sampler::Handle sampler) {
	return SaS::getSamplerVAddressing(sampler);
}
void setMagFilter(RE::Sampler::Handle sampler, RE::Sampler::FilteringModes mode) {
	SaS::setSamplerMagFilter(sampler, mode);
}
void setMinFilter(RE::Sampler::Handle sampler, RE::Sampler::FilteringModes mode) {
	SaS::setSamplerMinFilter(sampler, mode);
}
void setUAddressing(RE::Sampler::Handle sampler, RE::Sampler::AddressingModes mode) {
	SaS::setSamplerUAddressing(sampler, mode);
}
void setVAddressing(RE::Sampler::Handle sampler, RE::Sampler::AddressingModes mode) {
	SaS::setSamplerVAddressing(sampler, mode);
}
bool isValid(RE::Sampler::Handle sampler) {
	return SaS::isValid(sampler);
}
}; // namespace Sampler

// Shader wrappers (ShS)
namespace Shader {
RE::Shader::Handle create(const std::string &shader_file, const std::vector<RE::Shader::Definition> &defines) {
	return ShS::createShader(shader_file, defines);
}
void ref(RE::Shader::Handle shader) {
	ShS::refShader(shader);
}
void destroy(RE::Shader::Handle shader) {
	ShS::destroyShader(shader);
}
RE::Shader::Type getType(RE::Shader::Handle shader) {
	return ShS::getShaderType(shader);
}
bool isValid(RE::Shader::Handle shader) {
	return ShS::isValid(shader);
}
}; // namespace Shader

// Texture wrappers (TS)
namespace Texture {
Texture::Handle create(
		uint32_t width, uint32_t height,
		Texture::UsageFlags usage_flags,
		Texture::Format format) {
	return TS::createTexture(width, height, usage_flags, format);
}
void ref(Texture::Handle texture) {
	TS::refTexture(texture);
}
void destroy(Texture::Handle texture) {
	TS::destroyTexture(texture);
}
uint32_t getWidth(Texture::Handle texture) {
	return TS::getTextureWidth(texture);
}
uint32_t getHeight(Texture::Handle texture) {
	return TS::getTextureHeight(texture);
}
Texture::Format getFormat(Texture::Handle texture) {
	return TS::getTextureFormat(texture);
}
uint32_t getUsageFlags(Texture::Handle texture) {
	return TS::getTextureUsageFlags(texture);
}
RE::Sampler::Handle getSampler(RE::Texture::Handle texture) {
	return TS::getTextureSampler(texture);
}
void setWidth(RE::Texture::Handle texture, uint32_t width) {
	TS::setTextureWidth(texture, width);
}
void setHeight(RE::Texture::Handle texture, uint32_t height) {
	TS::setTextureHeight(texture, height);
}
void setFormat(RE::Texture::Handle texture, RE::Texture::Format format) {
	TS::setTextureFormat(texture, format);
}
void setUsageFlags(RE::Texture::Handle texture, uint32_t usage_flags) {
	TS::setTextureUsageFlags(texture, usage_flags);
}
void setSampler(RE::Texture::Handle texture, RE::Sampler::Handle sampler) {
	TS::setTextureSampler(texture, sampler);
}
void uploadBuffer(RE::Texture::Handle texture, std::shared_ptr<uint8_t> buffer, size_t offset, size_t count) {
	TS::uploadBufferToTexture(texture, buffer, offset, count);
}
bool isValid(RE::Texture::Handle texture) {
	return TS::isValid(texture);
}
}; // namespace Texture
}; // namespace RE