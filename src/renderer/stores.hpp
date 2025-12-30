#pragma once

#include "SDL3/SDL_gpu.h"
#include <SDL3/SDL.h>

#include <cstdint>
#include <cstring>
#include <renderer/common.hpp>
#include <renderer/handles.hpp>

struct StorageMetadata {
  uint32_t index;
  uint32_t generation;
  uint32_t ref_count;
};

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

struct Material {
  glm::vec4 color_factor;
  glm::vec3 emissive_factor;
  handle::Texture normal;
  handle::Texture emissive;
  handle::Texture occlusion;
  handle::Texture color;
  handle::Texture metallic_roughness;
  float normal_scale;
  float metallic_factor;
  float roughness_factor;
};

struct Mesh {
public:
  SDL_GPUBuffer *vert_buffer;
  SDL_GPUBuffer *index_buffer;
  uint32_t vert_count;
  uint32_t index_count;
  AABB aabb;
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

}; // namespace data
