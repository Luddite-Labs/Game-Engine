#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_stdinc.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <cstddef>

#include <renderer/renderer.hpp>

Renderer::Renderer(SDL_Window *_window) : m_window(_window), m_create_info() {
  SDL_assert(SDL_ShaderCross_Init());
  m_GPU_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  SDL_assert(SDL_ClaimWindowForGPUDevice(m_GPU_device, m_window));
  m_supported_shader_formats = SDL_GetGPUShaderFormats(m_GPU_device);

  Shader base_vertex_shader =
      loadShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.hlsl", 0, 0, 0, 0,
                 ShaderType::vertex);
  Shader base_frag_shader =
      loadShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.frag.hlsl", 0, 0, 0, 0,
                 ShaderType::fragment);

  SDL_GPUColorTargetDescription color_target_descriptions[] = {
      {.format = SDL_GetGPUSwapchainTextureFormat(m_GPU_device, m_window)}};
  SDL_GPUGraphicsPipelineTargetInfo target_info = {
      .color_target_descriptions = color_target_descriptions,
      .num_color_targets = 1};
  // Create the pipelines
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = m_shader_table[base_vertex_shader],
      .fragment_shader = m_shader_table[base_frag_shader],
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info = target_info,
  };
  pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  m_fill_pipeline =
      SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
  SDL_assert(m_fill_pipeline);
}

Renderer::~Renderer() { SDL_ShaderCross_Quit(); }

SDL_GPUDevice *Renderer::getGPUDevice() {
  return m_GPU_device;
}

// void Renderer::renderToWindow(Clay_RenderCommandArray draw_commands) {
//   SDL_GPUCommandBuffer *command_buffer =
//       SDL_AcquireGPUCommandBuffer(m_GPU_device);

//   SDL_GPUTexture *swapchain_texture;
//   SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(
//       command_buffer, m_window, &swapchain_texture, NULL, NULL));

//   SDL_GPUColorTargetInfo color_target_infos[] = {
//       {.texture = swapchain_texture,
//        .mip_level = 0,
//        .layer_or_depth_plane = 0,
//        .clear_color = {.r = 0.878f, .g = 0.816f, .b = 1.0f},
//        .load_op = SDL_GPU_LOADOP_CLEAR,
//        .store_op = SDL_GPU_STOREOP_STORE,
//        .resolve_texture = nullptr,
//        .resolve_mip_level = 0,
//        .resolve_layer = 0,
//        .cycle = false,
//        .cycle_resolve_texture = false}};

//   SDL_assert(command_buffer);
//   SDL_GPURenderPass *render_pass =
//       SDL_BeginGPURenderPass(command_buffer, color_target_infos, 1, nullptr);

//   SDL_BindGPUGraphicsPipeline(render_pass, m_fill_pipeline);

//   SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
//   SDL_EndGPURenderPass(render_pass);

//   SDL_SubmitGPUCommandBuffer(command_buffer);
// }

// void Renderer::renderToTexture(Clay_RenderCommandArray draw_commands,
//                                SDL_GPUTexture *target_texture, uint32_t
//                                width, uint32_t height) {
//   SDL_GPUColorTargetInfo color_target_infos[] = {
//       {.texture = target_texture,
//        .mip_level = 0,
//        .layer_or_depth_plane = 0,
//        .clear_color = {.r = 0.878f, .g = 0.816f, .b = 1.0f},
//        .load_op = SDL_GPU_LOADOP_CLEAR,
//        .store_op = SDL_GPU_STOREOP_STORE,
//        .resolve_texture = nullptr,
//        .resolve_mip_level = 0,
//        .resolve_layer = 0,
//        .cycle = false,
//        .cycle_resolve_texture = false}};

//   SDL_GPUCommandBuffer *command_buffer =
//       SDL_AcquireGPUCommandBuffer(m_GPU_device);
//   SDL_assert(command_buffer);
//   SDL_GPURenderPass *render_pass =
//       SDL_BeginGPURenderPass(command_buffer, color_target_infos, 1, nullptr);

//   SDL_BindGPUGraphicsPipeline(render_pass, m_fill_pipeline);

//   SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
//   SDL_EndGPURenderPass(render_pass);

//   SDL_SubmitGPUCommandBuffer(command_buffer);
// }

// add support for other formats
// change string type
// currently accepts hlsl shaders only

Shader Renderer::loadShader(const std::string &shader_file_path,
                            uint32_t num_samplers,
                            uint32_t num_storage_textures,
                            uint32_t num_storage_buffers,
                            uint32_t num_uniform_buffers,
                            Renderer::ShaderType shader_type) {
  SDL_assert(SDL_GetPathInfo(shader_file_path.c_str(), NULL));

  size_t data_size;
  void *buffer = SDL_LoadFile(shader_file_path.c_str(), &data_size);

  SDL_ShaderCross_ShaderStage shadercross_shader_stage;
  switch (shader_type) {
  case ShaderType::vertex:
    shadercross_shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    break;
  case ShaderType::fragment:
    shadercross_shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    break;
  }

  SDL_ShaderCross_HLSL_Info hlsl_info = {
      .source = reinterpret_cast<char *>(buffer),
      .entrypoint = "main",
      .include_dir = nullptr,
      .defines = nullptr,
      .shader_stage = shadercross_shader_stage,
      .enable_debug = true,
      .name = nullptr,
  };
  SDL_ShaderCross_GraphicsShaderMetadata shadercross_graphics_metadata = {
      .num_samplers = num_samplers,
      .num_storage_textures = num_storage_textures,
      .num_storage_buffers = num_storage_buffers,
      .num_uniform_buffers = num_uniform_buffers};

  SDL_GPUShader *shader = SDL_ShaderCross_CompileGraphicsShaderFromHLSL(
      m_GPU_device, &hlsl_info, &shadercross_graphics_metadata);
  SDL_assert(shader != nullptr);
  m_shader_table.push_back(shader);
  m_shader_type_record.push_back(shader_type);

  return m_shader_table.size() - 1;
}
// ! pick best format for platform?
// compiled source supported and file available
// ! explore using SDL_storage api
// std::string file_path;
// uint32_t selected_shader_format;
// if (m_supportedShaderFormats & SDL_GPU_SHADERFORMAT_SPIRV &&
//     SDL_GetPathInfo((shader_dir + shader_name + ".spirv").c_str(), NULL)) {
//   file_path = shader_dir + shader_name + ".spirv";
//   selected_shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
// } else if (m_supportedShaderFormats & SDL_GPU_SHADERFORMAT_DXBC &&
//            SDL_GetPathInfo((shader_dir + shader_name + ".dxbc").c_str(),
//                            NULL)) {
//   file_path = shader_dir + shader_name + ".dxbc";
//   selected_shader_format = SDL_GPU_SHADERFORMAT_DXBC;
// } else if (m_supportedShaderFormats & SDL_GPU_SHADERFORMAT_DXIL &&
//            SDL_GetPathInfo((shader_dir + shader_name + ".dxil").c_str(),
//                            NULL)) {
//   file_path = shader_dir + shader_name + ".dxil";
//   selected_shader_format = SDL_GPU_SHADERFORMAT_DXIL;
// } else if (m_supportedShaderFormats & SDL_GPU_SHADERFORMAT_MSL &&
//            SDL_GetPathInfo((shader_dir + shader_name + ".msl").c_str(),
//                            NULL)) {
//   file_path = shader_dir + shader_name + ".msl";
//   selected_shader_format = SDL_GPU_SHADERFORMAT_MSL;
// } else if (m_supportedShaderFormats & SDL_GPU_SHADERFORMAT_METALLIB &&
//            SDL_GetPathInfo((shader_dir + shader_name + ".metallib")
//                                .c_str(), // might not be the right
//                                extension
//                            NULL)) {
//   file_path = shader_dir + shader_name + ".metallib";
//   selected_shader_format = SDL_GPU_SHADERFORMAT_METALLIB;
// }

// if (!file_path.empty()) {
//   size_t data_size;
//   void *buffer = SDL_LoadFile(file_path.c_str(), &data_size);
//   SDL_GPUShaderCreateInfo create_info = {.code_size = data_size,
//                                          .code = buffer,
//                                          .entrypoint,
//                                          .format = selected_shader_format,
//                                          .stage,
//                                          .num_samplers,
//                                          .num_storage_textures,
//                                          .num_storage_buffers,
//                                          .num_uniform_buffers,
//                                          .props

//   };
//   SDL_GPUShader *shader = SDL_CreateGPUShader(m_GPUDevice, )
// }
// size_t data_size;
// void *buffer = SDL_LoadFile(file_path.c_str(), &data_size);
// SDL_GPUShaderCreateInfo create_info = {.code_size = data_size,
//                                         .code =
//                                         reinterpret_cast<uint8_t*>(buffer),
//                                         .entrypoint="main",
//                                         .format =
//                                         SDL_GPU_SHADERFORMAT_SPIRV,
//                                         .stage=SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX,
//                                         .num_samplers=0,
//                                         .num_storage_textures=0,
//                                         .num_storage_buffers=0,
//                                         .num_uniform_buffers=0
// };
// SDL_GPUShader *shader = SDL_CreateGPUShader(m_GPUDevice, &create_info);
// }

void Renderer::destroyShader(Shader &shader) {
  SDL_ReleaseGPUShader(m_GPU_device, m_shader_table[shader]);
  m_shader_table.erase(m_shader_table.begin() + shader);
  m_shader_type_record.erase(m_shader_type_record.begin() + shader);
  shader = -1;
}