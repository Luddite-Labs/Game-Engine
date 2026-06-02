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
enum Layers : u_int32_t {
	NONE = 0,
	ALBEDO = 1 << 0,
	VERTEX_NORMAL = 1 << 1,
	METALLIC = 1 << 2,
	ROUGHNESS = 1 << 3,
	EMISSIVE = 1 << 4,
	OCCLUSION = 1 << 5,
	SPECULAR = 1 << 6,
	DIFFUSE = 1 << 7,
	DIELECTRIC = 1 << 8,
	METALLIC_BRDF = 1 << 9,
	TANGENT = 1 << 10,
	NORMAL_TEXTURE = 1 << 11,
	SHADING_NORMAL = 1 << 12,
	UV = 1 << 13,
	DEPTH = 1 << 14,
	VERTEX_COLOR = 1 << 15,
	ALPHA = 1 << 16
};
void enable_bitset_enum(Layers);
struct Options {
	glm::vec4 clear_color;
	Layers layers;
};

namespace Buffer {
HANDLE();
enum Usage {
	VERTEX = SDL_GPU_BUFFERUSAGE_VERTEX,
	INDEX = SDL_GPU_BUFFERUSAGE_INDEX,
	INDIRECT = SDL_GPU_BUFFERUSAGE_INDIRECT,
	GRAPHICS_STORAGE_READ = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
	COMPUTE_STORAGE_READ = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
	COMPUTE_STORAGE_WRITE = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
};
struct Data {
	Usage usage;
	uint32_t size;
	SDL_GPUBuffer* gpu_handle;
};
}; // namespace Buffer

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

namespace Light {
HANDLE();

enum Type : uint8_t {
	DIRECTIONAL,
	POINT
};
struct Data {
	union {
		glm::vec3 position;
		glm::vec3 direction;
	};
	float padding1;
	glm::vec3 color;
	Type type;
	uint8_t padding3[3];
}; // std430 layout
}; // namespace Light

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
	REPEAT,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE
};
enum class FilteringModes : uint8_t { NEAREST,
	LINEAR };
enum class MipMapMode : uint8_t {
	NEAREST,
	LINEAR
};

struct Data {
	SDL_GPUSampler *gpu_handle;
	Sampler::FilteringModes mag_filter;
	Sampler::FilteringModes min_filter;
	Sampler::AddressingModes u_addressing;
	Sampler::AddressingModes v_addressing;
	Sampler::AddressingModes w_addressing;
	Sampler::MipMapMode mip_map_mode;
	bool enable_anisotropy;
};
}; // namespace Sampler

namespace Texture {
HANDLE();
enum class SampleCount : uint8_t {
	ONE,
	TWO,
	FOUR,
	EIGHT
};

enum class Format : uint8_t {
	R8G8B8A8_UNORM = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
	R8G8B8A8_UNORM_SRGB = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
	D16_UNORM = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	D24_UNORM = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
	D24_UNORM_S8_UINT = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
	D32_FLOAT = SDL_GPU_TEXTUREFORMAT_D32_FLOAT //! fix selection
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
	uint32_t mip_levels;
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
	Data(const Data &other) : pipeline(other.pipeline), type(other.type), vert_count(other.vert_count), index_count(other.index_count), material(other.material) {
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
enum AlphaModes : uint8_t {
	OPAQUE,
	MASK,
	BLEND,
};
struct Factors {
	glm::vec4 color_factor;
	glm::vec3 emissive_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
	float alpha_cutoff;
	AlphaModes alpha_mode;
	bool double_sided;
	uint8_t padding[2];
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
struct Definition {
	const char *name;
	const char *value;
};
struct Data {
	uint32_t num_samplers;
	uint32_t num_storage_textures;
	uint32_t num_storage_buffers;
	uint32_t num_uniform_buffers;
	Shader::Type type;
	std::string path;
	std::vector<RE::Shader::Definition> defines;
	SDL_GPUShader *gpu_handle;
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
	bool disable_back_face_culling;
	bool instanced;
	bool disable_depth_write;
	bool operator==(const Options &other) const {
		return (this->color_target_format == other.color_target_format) and
				(this->primitive_type == other.primitive_type) and
				(this->vert_attrs == other.vert_attrs) and
				(this->material_options == other.material_options) and
				(this->vert_shader.slot_index == other.vert_shader.slot_index) and
				(this->vert_shader.generation == other.vert_shader.generation) and
				(this->frag_shader.slot_index == other.frag_shader.slot_index) and
				(this->frag_shader.generation == other.frag_shader.generation) and
				(this->disable_back_face_culling == other.disable_back_face_culling) and
				(this->instanced == other.instanced) and
				(this->disable_depth_write == other.disable_depth_write);
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

inline const char *getString(RE::Sampler::AddressingModes mode) {
	switch (mode) {
		case RE::Sampler::AddressingModes::REPEAT:
			return "REPEAT";
		case RE::Sampler::AddressingModes::MIRRORED_REPEAT:
			return "MIRRORED_REPEAT";
		case RE::Sampler::AddressingModes::CLAMP_TO_EDGE:
			return "CLAMP_TO_EDGE";
	}
	return "";
}

inline const char *getString(RE::Shader::Type type) {
	switch (type) {
		case RE::Shader::Type::VERTEX:
			return "VERTEX";
		case RE::Shader::Type::FRAGMENT:
			return "FRAGMENT";
	}
	return "";
}

inline const char *getString(RE::Sampler::MipMapMode mode) {
	switch (mode) {
		case RE::Sampler::MipMapMode::LINEAR:
			return "LINEAR";
		case RE::Sampler::MipMapMode::NEAREST:
			return "NEAREST";
	}
	return "";
}

inline const char *getString(RE::Sampler::FilteringModes mode) {
	switch (mode) {
		case RE::Sampler::FilteringModes::LINEAR:
			return "LINEAR";
		case RE::Sampler::FilteringModes::NEAREST:
			return "NEAREST";
	}
	return "";
}

inline const char *getString(RE::Layers layers) {
	switch (layers) {
		case RE::Layers::NONE:
			return "NONE";
		case RE::Layers::ALBEDO:
			return "ALBEDO";
		case RE::Layers::VERTEX_NORMAL:
			return "VERTEX NORMAL";
		case RE::Layers::SHADING_NORMAL:
			return "SHADING NORMAL";
		case RE::Layers::NORMAL_TEXTURE:
			return "NORMAL TEXTURE";
		case RE::Layers::METALLIC:
			return "METALLIC";
		case RE::Layers::ROUGHNESS:
			return "ROUGHNESS";
		case RE::Layers::EMISSIVE:
			return "EMISSIVE";
		case RE::Layers::OCCLUSION:
			return "OCCLUSION";
		case RE::Layers::SPECULAR:
			return "SPECULAR";
		case RE::Layers::DIFFUSE:
			return "DIFFUSE";
		case RE::Layers::DIELECTRIC:
			return "DIELECTRIC";
		case RE::Layers::METALLIC_BRDF:
			return "METALLIC_BRDF";
		case RE::Layers::TANGENT:
			return "TANGENT";
		case RE::Layers::UV:
			return "UV";
		case RE::Layers::DEPTH:
			return "DEPTH";
		case RE::Layers::VERTEX_COLOR:
			return "VERTEX COLOR";
		case RE::Layers::ALPHA:
			return "ALPHA";
	}
	return "";
}

inline const char *getString(RE::Material::AlphaModes modes) {
	switch (modes) {
		case RE::Material::AlphaModes::OPAQUE:
			return "OPAQUE";
		case RE::Material::AlphaModes::MASK:
			return "MASK";
		case RE::Material::AlphaModes::BLEND:
			return "BLEND";
	}
	return "";
}

inline const char *getString(RE::Texture::Format format) {
	switch (format) {
		case RE::Texture::Format::R8G8B8A8_UNORM:
			return "R8G8B8A8_UNORM";
		case RE::Texture::Format::R8G8B8A8_UNORM_SRGB:
			return "R8G8B8A8_UNORM_SRGB";
		case RE::Texture::Format::D16_UNORM:
			return "D16_UNORM";
		case RE::Texture::Format::D24_UNORM:
			return "D24_UNORM";
		case RE::Texture::Format::D24_UNORM_S8_UINT:
			return "D24_UNORM_S8_UINT";
		case RE::Texture::Format::D32_FLOAT:
			return "D32_FLOAT";
	}
	return "";
}

inline const char *getString(RE::Mesh::Primitive::Type type) {
	switch (type) {
		case RE::Mesh::Primitive::Type::TRIANGLELIST:
			return "TRIANGLELIST";
	}
	return "";
}

inline const char *getString(RE::Vertex::Attributes type) {
	switch (type) {
		case RE::Vertex::Attributes::NONE:
			return "NONE";
		case RE::Vertex::Attributes::POSITION:
			return "POSITION";
		case RE::Vertex::Attributes::NORMAL:
			return "NORMAL";
		case RE::Vertex::Attributes::COLOR:
			return "COLOR";
		case RE::Vertex::Attributes::TANGENT:
			return "TANGENT";
		case RE::Vertex::Attributes::UV:
			return "UV";
		case RE::Vertex::Attributes::INDEX:
			return "INDEX";
	}

	return "";
}

inline const char *getString(RE::Material::Options option) {
	switch (option) {
		case RE::Material::Options::NONE:
			return "NONE";
		case RE::Material::Options::NORMAL_TEXTURE:
			return "NORMAL_TEXTURE";
		case RE::Material::Options::EMISSIVE_TEXTURE:
			return "EMISSIVE_TEXTURE";
		case RE::Material::Options::OCCLUSION_TEXTURE:
			return "OCCLUSION_TEXTURE";
		case RE::Material::Options::COLOR_TEXTURE:
			return "COLOR_TEXTURE";
		case RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE:
			return "METALLIC_ROUGHNESS_TEXTURE";
		case RE::Material::Options::COLOR_FACTOR_USED:
			return "COLOR_FACTOR_USED";
	}
	return "";
}

inline const char *getString(RE::Light::Type type) {
	switch (type) {
		case RE::Light::Type::DIRECTIONAL:
			return "DIRECTIONAL";
		case RE::Light::Type::POINT:
			return "POINT";
	}
	return "";
}