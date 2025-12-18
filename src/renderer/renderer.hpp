#pragma once
#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_gpu.h"
#include "glm/fwd.hpp"
#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <renderer/shader.hpp>

typedef struct Buffer Buffer;

struct Texture {
public:
  uintptr_t opq_handle; //! make private in future
  uint32_t width;
  uint32_t height;
  Texture() {}
  Texture(void *opq_handle, uint32_t width, uint32_t height)
      : opq_handle(reinterpret_cast<uintptr_t>(opq_handle)), width(width),
        height(height) {}
  friend class Renderer;
};

struct Camera {
public:
  bool is_orthogonal;
  glm::vec3 target;
  glm::vec3 position;
  glm::vec3 up;
  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;
  Camera()
      : fov(75.0f), aspectRatio(1.7777), nearPlane(1.0f), farPlane(100.0f),
        up(0, 1, 0), position({10.0f, 10.0f, 0.0f}), target(0.0f, 0.0f, 0.0f), is_orthogonal(false) {}
};

struct MeshData {
  // vertex position and normal interleaved data expected both float3
  void *vert_buffer;
  void *index_buffer;
  glm::mat4x4 model_to_world;
  Texture* uv_tex;
  uint32_t vert_count;
  uint32_t index_count;
};

struct MeshInternal {
  SDL_GPUBuffer* vert_buffer;
  SDL_GPUBuffer* index_buffer;
  SDL_GPUTexture* uv_tex;
  uint32_t vert_count;
  uint32_t index_count;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

typedef uint32_t Mesh;

class Renderer {
public:
  enum class ShaderType;
  glm::vec4 clear_color;

private:
  Shader m_base_vert_shader, m_base_frag_shader;
  SDL_Window *m_window;
  SDL_GPUDevice *m_GPU_device;
  SDL_GPUShaderFormat m_supported_shader_formats;
  SDL_GPUGraphicsPipeline *m_graphics_pipeline;
  SDL_GPUGraphicsPipelineCreateInfo m_create_info;
  std::vector<SDL_GPUShader *> m_shader_table;
  std::vector<ShaderType> m_shader_type_record;
  std::vector<MeshInternal> m_mesh_table;
  SDL_GPUGraphicsPipeline *m_fill_pipeline;
  Texture depth_buffer_texture;
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

  Texture createTexture(uint32_t width, uint32_t height);
  void destroyTexture(Texture *tex);

  Mesh createMesh(MeshData* mesh);
};