#pragma once
#include "SDL3/SDL_gpu.h"
#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

#include <renderer/shader.hpp>

class Renderer {
public:
  enum class ShaderType;

private:
  SDL_Window *m_window;
  SDL_GPUDevice *m_GPU_device;
  SDL_GPUShaderFormat m_supported_shader_formats;
  SDL_GPUGraphicsPipeline *m_graphics_pipeline;
  SDL_GPUGraphicsPipelineCreateInfo m_create_info;
  std::vector<SDL_GPUShader *> m_shader_table;
  std::vector<ShaderType> m_shader_type_record;
  SDL_GPUGraphicsPipeline *m_fill_pipeline;

public:
  enum class ShaderType { vertex, fragment };

  Renderer(SDL_Window *_window);
  ~Renderer();

  // pass shader without file extension engine will cross compile from format if
  // necessary
  Shader loadShader(const std::string &shader_file_path, uint32_t num_samplers,
                    uint32_t num_storage_textures, uint32_t num_storage_buffers,
                    uint32_t num_uniform_buffers, ShaderType shader_type);
  void destroyShader(Shader &shader);
  SDL_GPUDevice* getGPUDevice();
};