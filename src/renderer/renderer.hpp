#pragma once
#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_gpu.h"
#include "glm/fwd.hpp"
#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <renderer/shader.hpp>

typedef struct Buffer Buffer;

struct Texture {
public:
  uintptr_t opq_handle; //! make private in future
  uint32_t width;
  uint32_t height;
  Texture(void *opq_handle, uint32_t width, uint32_t height)
      : opq_handle(reinterpret_cast<uintptr_t>(opq_handle)), width(width),
        height(height) {}
  friend class Renderer;
};

struct Camera {
public:
  glm::vec3 target;
  glm::vec3 position;
  glm::vec3 up;
  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;
  Camera()
      : fov(75.0f), aspectRatio(1.7777), nearPlane(20.0f), farPlane(60.0f),
        up(0, 1, 0), position({30.0f, 30.0f, 0.0f}) {}
};

// struct Camera {
//   glm::mat4x4 pro
// };

class Renderer {
public:
  enum class ShaderType;

private:
  Shader m_base_vert_shader, m_base_frag_shader;
  SDL_Window *m_window;
  SDL_GPUDevice *m_GPU_device;
  SDL_GPUShaderFormat m_supported_shader_formats;
  SDL_GPUGraphicsPipeline *m_graphics_pipeline;
  SDL_GPUGraphicsPipelineCreateInfo m_create_info;
  std::vector<SDL_GPUShader *> m_shader_table;
  std::vector<ShaderType> m_shader_type_record;
  SDL_GPUGraphicsPipeline *m_fill_pipeline;
  SDL_GPUBuffer *mesh_vert_buffer;
  SDL_GPUBuffer *mesh_index_buffer;

  void uploadMeshData();

public:
  Camera camera;
  enum class ShaderType { vertex, fragment };

  Renderer(SDL_Window *_window);
  ~Renderer();

  // pass shader without file extension engine will cross compile from format if
  // necessary
  Shader loadShader(const std::string &shader_file_path, uint32_t num_samplers,
                    uint32_t num_storage_textures, uint32_t num_storage_buffers,
                    uint32_t num_uniform_buffers, ShaderType shader_type);
  void destroyShader(Shader &shader);
  SDL_GPUDevice *getGPUDevice();

  void draw();
  void drawToTexture(Texture *tex);

  Texture createTexture(uint32_t width, uint32_t height) {
    SDL_assert(m_window && m_GPU_device);
    const SDL_GPUTextureCreateInfo tex_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage =
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0};
    SDL_GPUTexture *tex = SDL_CreateGPUTexture(m_GPU_device, &tex_info);
    return Texture(tex, width, height);
  }
  void destroyTexture(Texture *tex) {
    SDL_assert(m_window && m_GPU_device);
    SDL_ReleaseGPUTexture(m_GPU_device,
                          reinterpret_cast<SDL_GPUTexture *>(tex->opq_handle));
    tex->opq_handle = reinterpret_cast<uintptr_t>(nullptr);
  }
};