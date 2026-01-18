#pragma once

#include "misc/utils.hpp"
#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>

#define HANDLE()                                                                              \
	struct Handle {                                                                           \
		uint32_t slot_index;                                                                  \
		uint32_t generation;                                                                  \
		bool operator==(const Handle &rhs) const {                                            \
			return this->slot_index == rhs.slot_index and this->generation == rhs.generation; \
		}                                                                                     \
		bool operator!=(const Handle &rhs) const {                                            \
			return !(*this == rhs);                                                           \
		}                                                                                     \
	};

namespace RE {
struct Options {
	glm::vec4 clear_color;
};

namespace Camera {
HANDLE();
struct Data {
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
}; // namespace Camera

namespace Vertex {
enum class Attributes : uint8_t {
	NONE = 0,
	POSITION = 1 << 0,
	NORMAL = 1 << 1,
	COLOR = 1 << 2,
	TANGENT = 1 << 3,
	UV = 1 << 4,
	INDEX = 1 << 5
};
enum class AttributeIndex : uint8_t {
	POSITION,
	NORMAL,
	TANGENT,
	COLOR,
	UV,
	INDEX,
	MAX
};
void enable_bitset_enum(Attributes);
}; // namespace Vertex

namespace Sampler {
HANDLE();
enum class AddressingModes : uint8_t {
	REPEAT = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	MIRRORED_REPEAT = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
	CLAMP_TO_EDGE = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
};
enum class FilteringModes : uint8_t { NEAREST = 0,
	LINEAR = 1 };

struct Data {
	Sampler::FilteringModes mag_filter;
	Sampler::FilteringModes min_filter;
	Sampler::AddressingModes u_addressing;
	Sampler::AddressingModes v_addressing;
	Sampler::AddressingModes w_addressing;
	SDL_GPUSampler *gpu_handle;
};
}; // namespace Sampler

namespace Texture {
HANDLE();

enum class Format : uint8_t {
	R8G8B8A8_UNORM = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
	D16_UNORM = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	D24_UNORM= SDL_GPU_TEXTUREFORMAT_D24_UNORM,
	D24_UNORM_S8_UINT= SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT
};
enum class Type : uint8_t {
	TEXTURE_2D = SDL_GPU_TEXTURETYPE_2D,
	TEXTURE_2D_ARRAY = SDL_GPU_TEXTURETYPE_2D_ARRAY,
	TEXTURE_3D = SDL_GPU_TEXTURETYPE_3D,
	TEXTURE_CUBE = SDL_GPU_TEXTURETYPE_CUBE,
	TEXTURE_CUBE_ARRAY = SDL_GPU_TEXTURETYPE_CUBE_ARRAY
};
enum class UsageFlags : uint8_t {
	SAMPLER = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	COLOR_TARGET = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
	DEPTH_STENCIL_TARGET = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
	GRAPHICS_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ,
	COMPUTE_STORAGE_READ = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ,
	COMPUTE_STORAGE_WRITE = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE,
	COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE =
			SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE,
};
void enable_bitset_enum(UsageFlags);
struct Data {
public:
	Sampler::Handle sampler;
	SDL_GPUTexture *gpu_handle;
	uint32_t width;
	uint32_t height;
	Texture::Format format;
	uint32_t usage_flags;
};

}; // namespace Texture

namespace Pipeline {
HANDLE();
};
namespace Material {
HANDLE();
};
namespace Mesh {
struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};
namespace Primitive {

enum class Type : uint8_t {
	TRIANGLELIST = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
};

struct AttributeData {
	std::unique_ptr<uint8_t[]> cpu_buffer;
	SDL_GPUBuffer *gpu_buffer;
};
struct Data {
	RE::Pipeline::Handle pipeline;
	Type type;
	AttributeData attrs_data[static_cast<uint32_t>(Vertex::AttributeIndex::MAX)];
	uint32_t vert_count;
	uint32_t index_count;
	RE::Material::Handle material;
	Data() : type(Type::TRIANGLELIST), attrs_data(), vert_count(0), index_count(0), material() {}
	Data(const Data &other) : pipeline(other.pipeline),  type(other.type), vert_count(other.vert_count), index_count(other.index_count), material(other.material) {
		for (uint32_t i = 0; i < static_cast<uint32_t>(Vertex::AttributeIndex::MAX); i++) {
			attrs_data[i].cpu_buffer.swap(const_cast<Data &>(other).attrs_data[i].cpu_buffer);
			attrs_data[i].gpu_buffer = other.attrs_data[i].gpu_buffer;
		}
	}
	void operator=(const Data &other) {
		pipeline = other.pipeline;
		type = other.type;
		vert_count = other.vert_count;
		index_count = other.index_count;
		material = other.material;
		for (uint32_t i = 0; i < static_cast<uint32_t>(Vertex::AttributeIndex::MAX); i++) {
			attrs_data[i].cpu_buffer.swap(const_cast<Data &>(other).attrs_data[i].cpu_buffer);
			attrs_data[i].gpu_buffer = other.attrs_data[i].gpu_buffer;
		}
	}
};
}; // namespace Primitive
HANDLE();
struct Data {
	std::vector<Primitive::Data> primitives;
	AABB aabb;
};
}; // namespace Mesh

namespace Material {
struct Factors {
	glm::vec4 color_factor;
	glm::vec3 emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
};
enum class Options : uint8_t {
	NONE = 0,
	NORMAL_TEXTURE = 1 << 0,
	EMISSIVE_TEXTURE = 1 << 1,
	OCCLUSION_TEXTURE = 1 << 2,
	COLOR_TEXTURE = 1 << 3,
	METALLIC_ROUGHNESS_TEXTURE = 1 << 4,
	COLOR_FACTOR_USED = 1 << 5,
};
void enable_bitset_enum(Options);
struct Data {
	Options options;
	Factors factors;
	RE::Texture::Handle normal;
	RE::Texture::Handle emissive;
	RE::Texture::Handle occlusion;
	RE::Texture::Handle color;
	RE::Texture::Handle metallic_roughness;
	std::string vert_shader_path;
	std::string frag_shader_path;
};
}; // namespace Material

namespace Shader {
enum class Type : uint8_t { VERTEX,
	FRAGMENT };
HANDLE();
struct Data {
	uint32_t num_samplers;
	uint32_t num_storage_textures;
	uint32_t num_storage_buffers;
	uint32_t num_uniform_buffers;
	Shader::Type type;
	SDL_GPUShader *gpu_handle;
};
struct Definition {
	const char *name;
	const char *value;
};
}; // namespace Shader

namespace Pipeline {
struct Options {
public:
	Texture::Format color_target_format;
	Mesh::Primitive::Type primitive_type;
	Vertex::Attributes vert_attrs;
	Material::Options material_options;
	Shader::Handle vert_shader;
	Shader::Handle frag_shader;
	bool instanced;
	bool operator==(const Options &other) const {
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

struct Data {
public:
	SDL_GPUGraphicsPipeline *gpu_handle;
	Options options;
};
}; // namespace Pipeline

}; // namespace RE

namespace RE::Mesh {
namespace Primitive {
struct Arg {
	Type type;
	std::unique_ptr<uint8_t[]> attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::MAX)];
	uint32_t vert_count;
	uint32_t index_count;
	RE::Material::Handle material;
};
}; // namespace Primitive
struct Arg {
	std::vector<Primitive::Arg> primitives;
	AABB aabb;
};
}; // namespace RE::Mesh
