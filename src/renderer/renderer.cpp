#include <algorithm>
#include <filesystem>
#include <renderer/renderer.hpp>
#include <span>
#include <string>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>

#include <renderer/graph.hpp>
#include <renderer/storage/camera-storage.hpp>
#include <renderer/storage/light-storage.hpp>
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
	glm::mat4x4 model;
	glm::mat4x4 view;
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
	RE::Material::Handle dummy_material;
	RE::Buffer::Handle light_buffer;
};

struct VertexUniformBuffer {
	TransformMatrices transform_matrices;
	RE::Layers layers;
	float padding[3];
};

struct FragmentUniformBuffer {
	RE::Material::Factors material;
	glm::mat4x4 inverted_camera_matrix;
	RE::Layers layers;
	float padding[3];
};

SDL_Window *m_window = nullptr;
SDL_GPUDevice *m_GPU_device = nullptr;
SDL_GPUGraphicsPipeline *m_mesh_pipeline = nullptr;
RE::Shader::Handle m_default_vert_shader = { 0, 0 };
RE::Shader::Handle m_default_frag_shader = { 0, 0 };
RendererData m_renderer_data;

void regenerateDepthTexture(
		uint32_t width,
		uint32_t height) {
	if (TS::isValid(m_renderer_data.depth_texture)) {
		TS::destroyTexture(m_renderer_data.depth_texture);
	}
	m_renderer_data.depth_texture = TS::createTexture(width, height,
			RE::Texture::UsageFlags::SAMPLER | RE::Texture::UsageFlags::DEPTH_STENCIL_TARGET,
			RE::Texture::Format::D24_UNORM, RE::Texture::SampleCount::ONE, false);
}

bool isMeshInViewFrustum(RE::Mesh::Handle mesh, glm::mat4x4 viewProj) {
	return true;
}
}; // namespace

namespace RE {

glm::mat4x4 getProjectionMatrix(const RE::Camera::Handle &camera) {
	if (CS::isCameraOrthogonal(camera)) {
		return glm::ortho(0.0f, CS::getCameraXMag(camera), 0.0f, CS::getCameraYMag(camera), CS::getCameraNearPlane(camera),
				CS::getCameraFarPlane(camera));
	} else {
		return glm::perspective(CS::getCameraFOV(camera), CS::getCameraAspectRatio(camera), CS::getCameraNearPlane(camera),
				CS::getCameraFarPlane(camera));
	}
}

void init() {
	m_GPU_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	RE::Graph::init(m_GPU_device);
	ShS::init(m_GPU_device);
	TS::init(m_GPU_device);
	SaS::init(m_GPU_device);
	PS::init(m_GPU_device);
	MaS::init(m_GPU_device);
	MS::init(m_GPU_device);
	LS::init(m_GPU_device);
	BS::init(m_GPU_device);
	m_renderer_data.dummy_texture = TS::createTexture(1, 1,
			RE::Texture::UsageFlags::SAMPLER,
			RE::Texture::Format::R8G8B8A8_UNORM, RE::Texture::SampleCount::ONE, false);
	TS::uploadBufferToTexture(m_renderer_data.dummy_texture,
			std::shared_ptr<uint8_t>(new uint8_t[4]{ 255, 255, 255, 255 },
					std::default_delete<uint8_t[]>()),
			0, 4);
	m_renderer_data.dummy_sampler = SaS::createSampler(
			RE::Sampler::FilteringModes::LINEAR,
			RE::Sampler::FilteringModes::LINEAR,
			RE::Sampler::AddressingModes::REPEAT,
			RE::Sampler::AddressingModes::REPEAT,
			RE::Sampler::AddressingModes::REPEAT,
			RE::Sampler::MipMapMode::LINEAR);
	m_renderer_data.depth_texture = TS::createTexture(1, 1,
			RE::Texture::UsageFlags::SAMPLER | RE::Texture::UsageFlags::DEPTH_STENCIL_TARGET,
			RE::Texture::Format::D24_UNORM, RE::Texture::SampleCount::ONE, false);
	m_renderer_data.dummy_material = MaS::createMaterial();
	m_renderer_data.light_buffer = BS::createBuffer(RE::Buffer::Usage::GRAPHICS_STORAGE_READ, LS::getMaxLightsCount() * sizeof(RE::Light::Data));
}

void destroy() {
	// destroy storages (reverse order)
	MS::destroy();
	MaS::destroy();
	PS::destroy();
	SaS::destroy();
	TS::destroy();
	ShS::destroy();
	LS::destroy();

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
	if (SaS::isValid(tex_sampler)) {
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
	FragmentUniformBuffer fragment_uniform_buffer{};
	VertexUniformBuffer vertex_uniform_buffer{};

	fragment_uniform_buffer.layers = renderer_options.layers;
	vertex_uniform_buffer.layers = renderer_options.layers;

	vertex_uniform_buffer.transform_matrices.proj = getProjectionMatrix(camera);
	vertex_uniform_buffer.transform_matrices.view = camera_transform;
	fragment_uniform_buffer.inverted_camera_matrix = glm::inverse(camera_transform);
	if (TS::getTextureWidth(target_texture) != TS::getTextureWidth(m_renderer_data.depth_texture) ||
			TS::getTextureHeight(target_texture) != TS::getTextureHeight(m_renderer_data.depth_texture)) {
		regenerateDepthTexture(
				TS::getTextureWidth(target_texture),
				TS::getTextureHeight(target_texture));
	}

	std::vector<PrimitiveRenderInfo> primitives;

	for (const auto draw_command : draw_commands) {
		if (not isMeshInViewFrustum(draw_command.mesh, vertex_uniform_buffer.transform_matrices.proj * camera_transform)) {
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
	// alpha sort
	sort(primitives.begin(), primitives.end(),
			[&](const auto &lhs, const auto &rhs) {
				return MaS::getMaterialAlphaMode(lhs.data->material) < MaS::getMaterialAlphaMode(rhs.data->material);
			});

	{
		RE::Graph::addPass("LIGHTS GPU TRANSFER", RE::Graph::PassType::COPY, [&](SDL_GPUCommandBuffer *command_buffer) {
			SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
			auto light_buffer = LS::getLightBuffer();
			auto light_buffer_size = light_buffer.size() * sizeof(RE::Light::Data);
			SDL_GPUTransferBufferCreateInfo transfer_create_info = {
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(light_buffer_size)
			};
			SDL_GPUTransferBuffer *transfer_buffer_handle = SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);
			void *transfer_buffer = SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer_handle, false);
			std::memcpy(transfer_buffer, light_buffer.data(), light_buffer_size);
			SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer_handle);
			SDL_GPUTransferBufferLocation transfer_buffer_location = {
				.transfer_buffer = transfer_buffer_handle,
				.offset = 0
			};
			SDL_GPUBufferRegion buffer_region = {
				.buffer = reinterpret_cast<SDL_GPUBuffer *>(RE::Graph::getDeviceHandle("LIGHT BUFFER")),
				.offset = 0,
				.size = static_cast<uint32_t>(light_buffer_size)
			};
			SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
			SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer_handle);
			SDL_EndGPUCopyPass(copy_pass);
		});
		RE::Graph::importBuffer("LIGHTS GPU TRANSFER", "LIGHT BUFFER", m_renderer_data.light_buffer);
		RE::Graph::writeBuffer("LIGHTS GPU TRANSFER", "LIGHT BUFFER");
	}

	{
		RE::Graph::addPass("SHADING PASS", RE::Graph::PassType::RENDER, [&](SDL_GPUCommandBuffer *command_buffer) {
			SDL_GPUColorTargetInfo color_target_infos[] = {
				{ .texture = reinterpret_cast<SDL_GPUTexture *>(RE::Graph::getDeviceHandle("FINAL RENDER TARGET")),
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
				.texture = reinterpret_cast<SDL_GPUTexture *>(RE::Graph::getDeviceHandle("DEPTH TARGET")),
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
			for (const auto &[transform, primitive, pipeline, material] : primitives) {
				//! group mesh by material
				const auto material_factors = MaS::getMaterialFactors(primitive->material);
				fragment_uniform_buffer.material = material_factors;
				SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform_buffer,
						sizeof(fragment_uniform_buffer));
				std::vector<SDL_GPUTextureSamplerBinding> sampler_bindings;
				sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
						.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
				if (TS::isValid(
							MaS::getMaterialColorTexture(primitive->material))) {
					sampler_bindings.back() = getSamplerBinding(
							MaS::getMaterialColorTexture(primitive->material));
				}
				sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
						.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
				if (TS::isValid(
							MaS::getMaterialNormalTexture(primitive->material))) {
					sampler_bindings.back() = getSamplerBinding(
							MaS::getMaterialNormalTexture(primitive->material));
				}
				sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
						.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
				if (TS::isValid(
							MaS::getMaterialEmissiveTexture(primitive->material))) {
					sampler_bindings.back() = getSamplerBinding(
							MaS::getMaterialEmissiveTexture(primitive->material));
				}
				sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
						.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
				if (TS::isValid(
							MaS::getMaterialMetallicRoughnessTexture(primitive->material))) {
					sampler_bindings.back() = getSamplerBinding(
							MaS::getMaterialMetallicRoughnessTexture(primitive->material));
				}
				sampler_bindings.push_back({ .texture = TS::getTextureGPUHandle(m_renderer_data.dummy_texture),
						.sampler = SaS::getSamplerGPUHandle(m_renderer_data.dummy_sampler) });
				if (TS::isValid(
							MaS::getMaterialOcclusionTexture(primitive->material))) {
					sampler_bindings.back() = getSamplerBinding(
							MaS::getMaterialOcclusionTexture(primitive->material));
				}
				SDL_BindGPUFragmentSamplers(render_pass, 0, sampler_bindings.data(),
						sampler_bindings.size());
				auto lights_buffer = reinterpret_cast<SDL_GPUBuffer *>(RE::Graph::getDeviceHandle("LIGHT BUFFER"));
				SDL_BindGPUFragmentStorageBuffers(render_pass, 0, &lights_buffer, 1);
				vertex_uniform_buffer.transform_matrices.model = transform;
				SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform_buffer,
						sizeof(vertex_uniform_buffer));

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
						if (primitive->index_count <= UINT16_MAX) {
							SDL_BindGPUIndexBuffer(render_pass, &index_binding,
									SDL_GPU_INDEXELEMENTSIZE_16BIT);
						} else {
							SDL_BindGPUIndexBuffer(render_pass, &index_binding,
									SDL_GPU_INDEXELEMENTSIZE_32BIT);
						}
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
		});
		RE::Graph::importBuffer("SHADING PASS", "LIGHT BUFFER", m_renderer_data.light_buffer);
		RE::Graph::readBuffer("SHADING PASS", "LIGHT BUFFER");
		RE::Graph::importTexture("SHADING PASS", "FINAL RENDER TARGET", target_texture);
		RE::Graph::writeTexture("SHADING PASS", "FINAL RENDER TARGET");
		RE::Graph::importTexture("SHADING PASS", "DEPTH TARGET", m_renderer_data.depth_texture);
		RE::Graph::writeTextureDepth("SHADING PASS", "DEPTH TARGET");
	}

	RE::Graph::execute();
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
float getAlphaCutoff(RE::Material::Handle material) {
	return MaS::getMaterialAlphaCutoff(material);
}
RE::Material::AlphaModes getAlphaMode(RE::Material::Handle material) {
	return MaS::getMaterialAlphaMode(material);
}
bool getDoubleSided(RE::Material::Handle material) {
	return MaS::getDoubleSided(material);
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
void setAlphaCutoff(RE::Material::Handle material, float alpha_cutoff) {
	MaS::setMaterialAlphaCutoff(material, alpha_cutoff);
}
void setAlphaMode(RE::Material::Handle material, RE::Material::AlphaModes alpha_mode) {
	MaS::setMaterialAlphaMode(material, alpha_mode);
}
void setDoubleSided(RE::Material::Handle material, bool double_sided) {
	MaS::setDoubleSided(material, double_sided);
}
bool isValid(RE::Material::Handle material) {
	return MaS::isValid(material);
}
}; // namespace Material

// Mesh wrappers (MS)
namespace Mesh {
RE::Mesh::Handle create(RE::Mesh::Arg &mesh_data) {
	for (auto &primitive : mesh_data.primitives) {
		if (not MaS::isValid(primitive.material)) {
			primitive.material = m_renderer_data.dummy_material;
		}
	}
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
		RE::Sampler::AddressingModes w_addressing,
		RE::Sampler::MipMapMode mip_map_mode,
		bool enable_anisotropy) {
	return SaS::createSampler(mag_filter, min_filter, u_addressing, v_addressing, w_addressing, mip_map_mode);
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
Sampler::AddressingModes getWAddressing(Sampler::Handle sampler) {
	return SaS::getSamplerWAddressing(sampler);
}
Sampler::MipMapMode getMipMapMode(Sampler::Handle sampler) {
	return SaS::getSamplerMipMapMode(sampler);
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
		Texture::Format format,
		RE::Texture::SampleCount sample_count,
		bool generate_mip_maps) {
	return TS::createTexture(width, height, usage_flags, format, RE::Texture::SampleCount::ONE, generate_mip_maps);
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

namespace Light {
void setPosition(Light::Handle light, const glm::vec3 &position) {
	LS::setPosition(light, position);
}
void setColor(Light::Handle light, const glm::vec3 &color) {
	LS::setColor(light, color);
}
glm::vec3 getPosition(Light::Handle light) {
	return LS::getPosition(light);
}
glm::vec3 getColor(Light::Handle light) {
	return LS::getColor(light);
}
Light::Handle create() {
	return LS::createLight();
}
std::span<RE::Light::Data> getBuffer() {
	return LS::getLightBuffer();
}
void ref(Light::Handle light) {
	LS::refLight(light);
}
void destroy(Light::Handle light) {
	LS::destroyLight(light);
}
bool isValid(Light::Handle light) {
	return LS::isValid(light);
}

}; // namespace Light
}; // namespace RE