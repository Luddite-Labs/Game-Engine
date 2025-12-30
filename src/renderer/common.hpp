#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <renderer/common.hpp>

enum class SamplerAddressingModes : uint8_t {
  REPEAT = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  MIRRORED_REPEAT = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
  CLAMP_TO_EDGE = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
};
enum class SamplerFilteringModes : uint8_t { NEAREST = 0, LINEAR = 1 };
enum class TextureFormat : uint8_t {
  R8G8B8A8_UNORM = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
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
enum class ShaderType : uint8_t { VERTEX, FRAGMENT };

void enable_bitset_enum(TextureUsageFlags);

struct AABB {
  glm::vec2 x;
  glm::vec2 y;
  glm::vec2 z;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};