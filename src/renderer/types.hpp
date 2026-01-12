#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>

enum class SamplerAddressingModes : uint8_t {
	REPEAT = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	MIRRORED_REPEAT = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
	CLAMP_TO_EDGE = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
};
enum class SamplerFilteringModes : uint8_t { NEAREST = 0,
	LINEAR = 1 };
enum class TextureFormat : uint8_t {
	R8G8B8A8_UNORM = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
	D16_UNORM = SDL_GPU_TEXTUREFORMAT_D16_UNORM
};
enum class TextureType : uint8_t {
	TEXTURE_2D = SDL_GPU_TEXTURETYPE_2D,
	TEXTURE_2D_ARRAY = SDL_GPU_TEXTURETYPE_2D_ARRAY,
	TEXTURE_3D = SDL_GPU_TEXTURETYPE_3D,
	TEXTURE_CUBE = SDL_GPU_TEXTURETYPE_CUBE,
	TEXTURE_CUBE_ARRAY = SDL_GPU_TEXTURETYPE_CUBE_ARRAY
};
enum class TextureUsageFlags : uint8_t {
	SAMPLER = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	COLOR_TARGET = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
	DEPTH_STENCIL_TARGET = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
	GRAPHICS_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
	COMPUTE_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ,
	COMPUTE_STORAGE_WRITE = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE,
	COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE =
			SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE,
};
enum class ShaderType : uint8_t { VERTEX,
	FRAGMENT };

void enable_bitset_enum(TextureUsageFlags);

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct RendererOptions {
	glm::vec4 clear_color;
};

#include "misc/utils.hpp"

namespace handle {
#ifndef GAME_ENGINE_DEBUG_MODE
struct Camera {
	Handle handle;
};
#else
typedef Handle Camera;
#endif

#ifndef GAME_ENGINE_DEBUG_MODE
struct Sampler {
	Handle handle;
};
#else
typedef Handle Sampler;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Texture {
	Handle handle;
};
#else
typedef Handle Texture;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Mesh {
	Handle handle;
};
#else
typedef Handle Mesh;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Material {
	Handle handle;
};
#else
typedef Handle Material;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Shader {
	Handle handle;
};
#else
typedef Handle Shader;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Pipeline {
	Handle handle;
};
#else
typedef Handle Pipeline;
#endif
} // namespace handle

namespace data {

struct Camera { // instant updates
	union {
		float fov;
		float xmag;
	};
	union {
		float aspect_ratio;
		float ymag;
	};
	float near_plane;
	float far_plane;
	bool is_orthogonal;
};

enum class MaterialOptions : uint8_t {
	NONE = 0,
	NORMAL_TEXTURE = 1 << 0,
	EMISSIVE_TEXTURE = 1 << 1,
	OCCLUSION_TEXTURE = 1 << 2,
	COLOR_TEXTURE = 1 << 3,
	METALLIC_ROUGHNESS_TEXTURE = 1 << 4,
	COLOR_FACTOR_USED = 1 << 5,
};

struct MaterialFactors {
	glm::vec4 color_factor;
	glm::vec3 emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
};

struct Material {
	MaterialOptions options;
	MaterialFactors factors;
	handle::Texture normal;
	handle::Texture emissive;
	handle::Texture occlusion;
	handle::Texture color;
	handle::Texture metallic_roughness;
	std::string vert_shader_path;
	std::string frag_shader_path;
};

enum class VertAttributes : uint8_t {
	NONE = 0,
	POSITION = 1 << 0,
	NORMAL = 1 << 1,
	COLOR = 1 << 2,
	TANGENT = 1 << 3,
	UV = 1 << 4,
	INDEX = 1 << 5
};

enum class VertAttributeIndex : uint8_t {
	POSITION,
	NORMAL,
	COLOR,
	TANGENT,
	UV,
	INDEX,
	MAX
};

enum class PrimitiveType : uint8_t {
	TRIANGLELIST = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
};

struct PrimitiveAttrData {
	std::unique_ptr<uint8_t[]> cpu_buffer;
	SDL_GPUBuffer *gpu_buffer;
};

struct PrimitiveData {
	PrimitiveType type;
	std::unique_ptr<uint8_t[]> attrs_data[static_cast<uint32_t>(VertAttributeIndex::MAX)];
	uint32_t vert_count;
	uint32_t index_count;
	handle::Material material;
};

struct MeshData {
	std::vector<PrimitiveData> primitives;
	AABB aabb;
};

struct Primitive {
	handle::Pipeline pipeline;
	PrimitiveType type;
	PrimitiveAttrData attrs_data[static_cast<uint32_t>(VertAttributeIndex::MAX)];
	uint32_t vert_count;
	uint32_t index_count;
	handle::Material material;
	Primitive() : type(PrimitiveType::TRIANGLELIST), attrs_data(), vert_count(0), index_count(0), material() {}
	Primitive(const Primitive &other) : type(other.type), vert_count(other.vert_count), index_count(other.index_count), material(other.material) {
		for (uint32_t i = 0; i < static_cast<uint32_t>(VertAttributeIndex::MAX); i++) {
			attrs_data[i].cpu_buffer.swap(const_cast<Primitive &>(other).attrs_data[i].cpu_buffer);
			attrs_data[i].gpu_buffer = other.attrs_data[i].gpu_buffer;
		}
	}
	void operator=(const Primitive &other) {
		type = other.type;
		vert_count = other.vert_count;
		index_count = other.index_count;
		material = other.material;
		for (uint32_t i = 0; i < static_cast<uint32_t>(VertAttributeIndex::MAX); i++) {
			attrs_data[i].cpu_buffer.swap(const_cast<Primitive &>(other).attrs_data[i].cpu_buffer);
			attrs_data[i].gpu_buffer = other.attrs_data[i].gpu_buffer;
		}
	}
};

struct Mesh {
	std::vector<Primitive> primitives;
	AABB aabb;
	Mesh() : primitives(), aabb({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }) {}
	Mesh(const Mesh &other) : primitives(std::move(other.primitives)), aabb(other.aabb) {}
};

void enable_bitset_enum(VertAttributes);
void enable_bitset_enum(MaterialOptions);

struct PipelineOptions {
public:
	TextureFormat color_target_format;
	PrimitiveType primitive_type;
	VertAttributes vert_attrs;
	MaterialOptions material_options;
	handle::Shader vert_shader;
	handle::Shader frag_shader;
	bool instanced;
	bool operator==(const PipelineOptions &other) const {
		return (this->color_target_format == other.color_target_format) and
				(this->primitive_type == other.primitive_type) and
				(this->vert_attrs == other.vert_attrs) and
				(this->material_options == other.material_options) and
				(this->vert_shader.slot_index == other.vert_shader.slot_index) and
				(this->vert_shader.generation == other.vert_shader.generation) and
				(this->frag_shader.slot_index == other.frag_shader.slot_index) and
				(this->frag_shader.generation == other.frag_shader.generation) and
				(this->instanced == other.instanced);
	}
};

struct Pipeline {
public:
	SDL_GPUGraphicsPipeline *gpu_handle;
	PipelineOptions options;
};

struct Texture {
public:
	handle::Sampler sampler;
	SDL_GPUTexture *gpu_handle;
	uint32_t width;
	uint32_t height;
	TextureFormat format;
	uint32_t usage_flags;
};

struct Sampler {
	SamplerFilteringModes mag_filter;
	SamplerFilteringModes min_filter;
	SamplerAddressingModes u_addressing;
	SamplerAddressingModes v_addressing;
	SDL_GPUSampler *gpu_handle;
};

struct Shader {
	uint32_t num_samplers;
	uint32_t num_storage_textures;
	uint32_t num_storage_buffers;
	uint32_t num_uniform_buffers;
	ShaderType type;
	SDL_GPUShader *gpu_handle;
};

struct ShaderDefinition {
	const char *name;
	const char *value;
};
}; // namespace data