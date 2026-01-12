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
	const data::Primitive *data;
	handle::Pipeline pipeline;
	handle::Material material;
};

struct RendererData {
	handle::Texture dummy_texture;
	handle::Texture depth_texture;
	handle::Sampler dummy_sampler;
};

SDL_Window *m_window = nullptr;
SDL_GPUDevice *m_GPU_device = nullptr;
SDL_GPUGraphicsPipeline *m_mesh_pipeline = nullptr;
handle::Shader m_default_vert_shader = { 0, 0 };
handle::Shader m_default_frag_shader = { 0, 0 };
RendererData m_renderer_data;

glm::mat4x4 getProjectionMatrix(const handle::Camera &camera) {
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
			TextureUsageFlags::SAMPLER | TextureUsageFlags::DEPTH_STENCIL_TARGET,
			TextureFormat::D16_UNORM);
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
			TextureUsageFlags::SAMPLER,
			TextureFormat::R8G8B8A8_UNORM);
	TS::uploadBufferToTexture(m_renderer_data.dummy_texture,
			std::shared_ptr<uint8_t>(new uint8_t[4]{ 255, 255, 255, 255 },
					std::default_delete<uint8_t[]>()),
			0, 4);
	m_renderer_data.dummy_sampler = SaS::createSampler(
			SamplerFilteringModes::NEAREST,
			SamplerFilteringModes::NEAREST,
			SamplerAddressingModes::CLAMP_TO_EDGE,
			SamplerAddressingModes::CLAMP_TO_EDGE);
	m_renderer_data.depth_texture = TS::createTexture(1, 1,
			TextureUsageFlags::SAMPLER | TextureUsageFlags::DEPTH_STENCIL_TARGET,
			TextureFormat::D16_UNORM);
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
		const handle::Texture &texture_handle) {
	SDL_GPUTextureSamplerBinding binding = {
		.texture = TS::getTextureGPUHandle(texture_handle),
		.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler)
	};
	handle::Sampler tex_sampler = TS::getTextureSampler(texture_handle);
	if (SamplerStorageType::isValid(tex_sampler)) {
		binding.sampler = SaS::getSamplerGPUHandle(tex_sampler);
	}
	return binding;
}

//! infinite projection
void drawToTexture(const RendererOptions &renderer_options,
		const handle::Texture &target_texture,
		const handle::Camera &camera,
		const glm::mat4x4 &camera_transform,
		std::vector<DrawCommand> &draw_commands) {
	if (draw_commands.size() == 0) {
		return;
	}

	if (TS::getTextureWidth(target_texture) != TS::getTextureWidth(m_renderer_data.depth_texture) ||
			TS::getTextureHeight(target_texture) != TS::getTextureHeight(m_renderer_data.depth_texture)) {
		regenerateDepthTexture(
				TS::getTextureWidth(target_texture),
				TS::getTextureHeight(target_texture));
		return;
	}

	std::vector<PrimitiveRenderInfo> primitives;

	for (const auto draw_command : draw_commands) {
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
	TransformMatrices transform_matrices;
	transform_matrices.proj = getProjectionMatrix(camera);
	for (const auto [transform, primitive, pipeline, material] : primitives) {
		//! group mesh by material
		if (SlotMap<std::vector<data::Material>, data::Material>::isValid(
					primitive->material)) {
			const auto material_factors = MaS::getMaterialFactors(primitive->material);
			SDL_PushGPUFragmentUniformData(command_buffer, 0, &material_factors,
					sizeof(data::MaterialFactors));
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
				i < static_cast<uint32_t>(data::VertAttributeIndex::MAX); i++) {
			if (static_cast<data::VertAttributeIndex>(i) ==
							data::VertAttributeIndex::INDEX and
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
uintptr_t getTexture(handle::Texture texture) {
	return reinterpret_cast<uintptr_t>(TS::getTextureGPUHandle(texture));
}

// Camera wrappers (CS)
handle::Camera createCamera() {
	return CS::createCamera();
}
void refCamera(handle::Camera camera) {
	CS::refCamera(camera);
}
void destroyCamera(handle::Camera camera) {
	CS::destroyCamera(camera);
}
void setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
		float fov, float near_plane,
		float far_plane) {
	CS::setPerspectiveCamera(camera, aspect_ratio, fov, near_plane, far_plane);
}
void setOrthogonalCamera(handle::Camera camera, float xmag,
		float ymag, float near_plane,
		float far_plane) {
	CS::setOrthogonalCamera(camera, xmag, ymag, near_plane, far_plane);
}
float getCameraAspectRatio(handle::Camera camera) {
	return CS::getCameraAspectRatio(camera);
}
float getCameraFOV(handle::Camera camera) {
	return CS::getCameraFOV(camera);
}
float getCameraXMag(handle::Camera camera) {
	return CS::getCameraXMag(camera);
}
float getCameraYMag(handle::Camera camera) {
	return CS::getCameraYMag(camera);
}
float getCameraNearPlane(handle::Camera camera) {
	return CS::getCameraNearPlane(camera);
}
float getCameraFarPlane(handle::Camera camera) {
	return CS::getCameraFarPlane(camera);
}
bool isCameraOrthogonal(handle::Camera camera) {
	return CS::isCameraOrthogonal(camera);
}
void setCameraAspectRatio(handle::Camera camera, float aspect_ratio) {
	CS::setCameraAspectRatio(camera, aspect_ratio);
}
void setCameraFOV(handle::Camera camera, float fov) {
	CS::setCameraFOV(camera, fov);
}
void setCameraXMag(handle::Camera camera, float xmag) {
	CS::setCameraXMag(camera, xmag);
}
void setCameraYMag(handle::Camera camera, float ymag) {
	CS::setCameraYMag(camera, ymag);
}
void setCameraNearPlane(handle::Camera camera, float near_plane) {
	CS::setCameraNearPlane(camera, near_plane);
}
void setCameraFarPlane(handle::Camera camera, float far_plane) {
	CS::setCameraFarPlane(camera, far_plane);
}
void setCameraIsOrthogonal(handle::Camera camera, bool is_orthogonal) {
	CS::setCameraIsOrthogonal(camera, is_orthogonal);
}

// Material wrappers (MaS)
handle::Material createMaterial() {
	return MaS::createMaterial();
}
void refMaterial(handle::Material material) {
	MaS::refMaterial(material);
}
void destroyMaterial(handle::Material material) {
	MaS::destroyMaterial(material);
}
data::MaterialOptions getMaterialOptions(handle::Material material) {
	return MaS::getMaterialOptions(material);
}
glm::vec4 getMaterialColorFactor(handle::Material material) {
	return MaS::getMaterialColorFactor(material);
}
glm::vec3 getMaterialEmissiveFactor(handle::Material material) {
	return MaS::getMaterialEmissiveFactor(material);
}
handle::Texture getMaterialNormalTexture(handle::Material material) {
	return MaS::getMaterialNormalTexture(material);
}
handle::Texture getMaterialEmissiveTexture(handle::Material material) {
	return MaS::getMaterialEmissiveTexture(material);
}
handle::Texture getMaterialOcclusionTexture(handle::Material material) {
	return MaS::getMaterialOcclusionTexture(material);
}
handle::Texture getMaterialColorTexture(handle::Material material) {
	return MaS::getMaterialColorTexture(material);
}
handle::Texture getMaterialMetallicRoughnessTexture(handle::Material material) {
	return MaS::getMaterialMetallicRoughnessTexture(material);
}
float getMaterialNormalScale(handle::Material material) {
	return MaS::getMaterialNormalScale(material);
}
float getMaterialMetallicFactor(handle::Material material) {
	return MaS::getMaterialMetallicFactor(material);
}
float getMaterialRoughnessFactor(handle::Material material) {
	return MaS::getMaterialRoughnessFactor(material);
}
void setMaterialColorFactor(handle::Material material, glm::vec4 color_factor) {
	MaS::setMaterialColorFactor(material, color_factor);
}
void setMaterialEmissiveFactor(handle::Material material, glm::vec3 emissive_factor) {
	MaS::setMaterialEmissiveFactor(material, emissive_factor);
}
void setMaterialNormalTexture(handle::Material material, handle::Texture normal) {
	MaS::setMaterialNormalTexture(material, normal);
}
void setMaterialEmissiveTexture(handle::Material material, handle::Texture emissive) {
	MaS::setMaterialEmissiveTexture(material, emissive);
}
void setMaterialOcclusionTexture(handle::Material material, handle::Texture occlusion) {
	MaS::setMaterialOcclusionTexture(material, occlusion);
}
void setMaterialColorTexture(handle::Material material, handle::Texture color) {
	MaS::setMaterialColorTexture(material, color);
}
void setMaterialMetallicRoughness(handle::Material material, handle::Texture metallic_roughness) {
	MaS::setMaterialMetallicRoughness(material, metallic_roughness);
}
void setMaterialNormalScale(handle::Material material, float normal_scale) {
	MaS::setMaterialNormalScale(material, normal_scale);
}
void setMaterialMetallicFactor(handle::Material material, float metallic_factor) {
	MaS::setMaterialMetallicFactor(material, metallic_factor);
}
void setMaterialRoughnessFactor(handle::Material material, float roughness_factor) {
	MaS::setMaterialRoughnessFactor(material, roughness_factor);
}

// Mesh wrappers (MS)
handle::Mesh createMesh(data::MeshData &mesh_data) {
	return MS::createMesh(mesh_data);
}
void refMesh(handle::Mesh mesh) {
	MS::refMesh(mesh);
}
void destroyMesh(handle::Mesh mesh) {
	MS::destroyMesh(mesh);
}
AABB getMeshAABB(handle::Mesh mesh) {
	return MS::getMeshAABB(mesh);
}
const data::Primitive &getPrimitiveData(handle::Mesh mesh, uint32_t primitive_index) {
	return MS::getPrimitiveData(mesh, primitive_index);
}
void setMeshAABB(handle::Mesh mesh, AABB aabb) {
	MS::setMeshAABB(mesh, aabb);
}
const std::vector<data::Primitive> &getMeshPrimitives(handle::Mesh mesh) {
	return MS::getMeshPrimitives(mesh);
}

// Pipeline wrappers (PS)
handle::Pipeline createPipeline(const data::PipelineOptions &options) {
	return PS::createPipeline(options);
}

// Sampler wrappers (SaS)
handle::Sampler createSampler(SamplerFilteringModes mag_filter, SamplerFilteringModes min_filter, SamplerAddressingModes u_addressing, SamplerAddressingModes v_addressing) {
	return SaS::createSampler(mag_filter, min_filter, u_addressing, v_addressing);
}
void refSampler(handle::Sampler sampler) {
	SaS::refSampler(sampler);
}
void destroySampler(handle::Sampler sampler) {
	SaS::destroySampler(sampler);
}
SamplerFilteringModes getSamplerMagFilter(handle::Sampler sampler) {
	return SaS::getSamplerMagFilter(sampler);
}
SamplerFilteringModes getSamplerMinFilter(handle::Sampler sampler) {
	return SaS::getSamplerMinFilter(sampler);
}
SamplerAddressingModes getSamplerUAddressing(handle::Sampler sampler) {
	return SaS::getSamplerUAddressing(sampler);
}
SamplerAddressingModes getSamplerVAddressing(handle::Sampler sampler) {
	return SaS::getSamplerVAddressing(sampler);
}
void setSamplerMagFilter(handle::Sampler sampler, SamplerFilteringModes mode) {
	SaS::setSamplerMagFilter(sampler, mode);
}
void setSamplerMinFilter(handle::Sampler sampler, SamplerFilteringModes mode) {
	SaS::setSamplerMinFilter(sampler, mode);
}
void setSamplerUAddressing(handle::Sampler sampler, SamplerAddressingModes mode) {
	SaS::setSamplerUAddressing(sampler, mode);
}
void setSamplerVAddressing(handle::Sampler sampler, SamplerAddressingModes mode) {
	SaS::setSamplerVAddressing(sampler, mode);
}

// Shader wrappers (ShS)
handle::Shader createShader(const std::string &shader_file, const std::vector<data::ShaderDefinition> &defines) {
	return ShS::createShader(shader_file, defines);
}
void refShader(handle::Shader shader) {
	ShS::refShader(shader);
}
void destroyShader(handle::Shader shader) {
	ShS::destroyShader(shader);
}
ShaderType getShaderType(handle::Shader shader) {
	return ShS::getShaderType(shader);
}
uint32_t getShaderNumSamplers(handle::Shader shader) {
	return ShS::getShaderNumSamplers(shader);
}
uint32_t getShaderNumStorageTextures(handle::Shader shader) {
	return ShS::getShaderNumStorageTextures(shader);
}
uint32_t getShaderNumStorageBuffers(handle::Shader shader) {
	return ShS::getShaderNumStorageBuffers(shader);
}
uint32_t getShaderNumUniformBuffers(handle::Shader shader) {
	return ShS::getShaderNumUniformBuffers(shader);
}
SDL_GPUShader *getShaderGPUHandle(handle::Shader shader) {
	return ShS::getShaderGPUHandle(shader);
}

// Texture wrappers (TS)
handle::Texture createTexture(uint32_t width, uint32_t height, TextureUsageFlags usage_flags, TextureFormat format) {
	return TS::createTexture(width, height, usage_flags, format);
}
void refTexture(handle::Texture texture) {
	TS::refTexture(texture);
}
void destroyTexture(handle::Texture texture) {
	TS::destroyTexture(texture);
}
uint32_t getTextureWidth(handle::Texture texture) {
	return TS::getTextureWidth(texture);
}
uint32_t getTextureHeight(handle::Texture texture) {
	return TS::getTextureHeight(texture);
}
TextureFormat getTextureFormat(handle::Texture texture) {
	return TS::getTextureFormat(texture);
}
uint32_t getTextureUsageFlags(handle::Texture texture) {
	return TS::getTextureUsageFlags(texture);
}
handle::Sampler getTextureSampler(handle::Texture texture) {
	return TS::getTextureSampler(texture);
}
void setTextureWidth(handle::Texture texture, uint32_t width) {
	TS::setTextureWidth(texture, width);
}
void setTextureHeight(handle::Texture texture, uint32_t height) {
	TS::setTextureHeight(texture, height);
}
void setTextureFormat(handle::Texture texture, TextureFormat format) {
	TS::setTextureFormat(texture, format);
}
void setTextureUsageFlags(handle::Texture texture, uint32_t usage_flags) {
	TS::setTextureUsageFlags(texture, usage_flags);
}
void setTextureSampler(handle::Texture texture, handle::Sampler sampler) {
	TS::setTextureSampler(texture, sampler);
}
void uploadBufferToTexture(handle::Texture texture, std::shared_ptr<uint8_t> buffer, size_t offset, size_t count) {
	TS::uploadBufferToTexture(texture, buffer, offset, count);
}
}; // namespace RE