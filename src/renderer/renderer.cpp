#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <misc/log.hpp>
#include <renderer/renderer.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"

glm::mat4x4 generateViewProjMatrix(Camera *camera) {
  glm::vec3 forward = glm::normalize(camera->target - camera->position);
  glm::vec3 W = -forward;
  glm::vec3 U = glm::normalize(glm::cross(camera->up, W));
  glm::vec3 V = glm::normalize(glm::cross(W, U));
  glm::mat4x4 view(glm::vec4(U.x, V.x, W.x, 0.0f),

                   glm::vec4(U.y, V.y, W.y, 0.0f),

                   glm::vec4(U.z, V.z, W.z, 0.0f),

                   glm::vec4(-glm::dot(U, camera->position),
                             -glm::dot(V, camera->position),
                             -glm::dot(W, camera->position), 1.0f));
  float radians = (camera->fov * SDL_PI_F) / 180.0f;
  float fovFactor = 1.0f / SDL_tanf(radians / 2.0f);
  float dist = camera->farPlane - camera->nearPlane;
  float lambda = (camera->farPlane / dist);
  glm::mat4x4 proj;
  if (camera->is_orthogonal) {
    proj = glm::mat4x4(2 / (100.0f * camera->aspectRatio), 0, 0, 0,

                       0, 2 / (100.0f), 0, 0,

                       0, 0, -2 / (camera->farPlane - camera->nearPlane), 0,

                       0, 0,
                       -(camera->farPlane + camera->nearPlane) /
                           (camera->farPlane - camera->nearPlane),
                       1);
  } else {
    proj = glm::mat4x4(fovFactor / camera->aspectRatio, 0, 0, 0,

                       0, fovFactor, 0, 0,

                       0, 0, -lambda, -1,

                       0, 0, lambda * camera->nearPlane, 0);
  }
  glm::mat4x4 viewproj = proj * view;
  return viewproj;
  // return {0.617629349,
  //         -0.714224815,
  //         -0.822066069,
  //         -0.548044026,
  //         0,
  //         0.921519339,
  //         -1.06066012,
  //         -0.707106769,
  //         -0.757549822,
  //         -0.582306504,
  //         -0.670229375,
  //         -0.446819574,
  //         0,
  //         1.24285248e-06,
  //         33.6396103,
  //         42.4264069};
}

Renderer::Renderer(SDL_Window *_window)
    : m_window(_window), m_create_info(), mesh_vert_buffer(nullptr),
      mesh_index_buffer(nullptr), camera() {
  SDL_assert(SDL_ShaderCross_Init());
  m_GPU_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  SDL_assert(SDL_ClaimWindowForGPUDevice(m_GPU_device, m_window));
  m_supported_shader_formats = SDL_GetGPUShaderFormats(m_GPU_device);
  m_base_vert_shader =
      loadShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.hlsl", 0, 0, 0, 1,
                 ShaderType::vertex);
  m_base_frag_shader =
      loadShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.frag.hlsl", 0, 0, 0, 0,
                 ShaderType::fragment);

  SDL_GPUColorTargetDescription color_target_descriptions[] = {
      {.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM}};
  SDL_GPUGraphicsPipelineTargetInfo target_info = {
      .color_target_descriptions = color_target_descriptions,
      .num_color_targets = 1};
  // Create the pipelines
  SDL_GPUVertexAttribute vert_attrs[] = {
      {.location = 0,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
       .offset = 0}};
  SDL_GPUVertexBufferDescription vert_buffer_descriptions[] = {
      {.slot = 0,
       .pitch = sizeof(float) * 3,
       .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
       .instance_step_rate = 0}};
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = m_shader_table[m_base_vert_shader],
      .fragment_shader = m_shader_table[m_base_frag_shader],
      .vertex_input_state = {.vertex_buffer_descriptions =
                                 vert_buffer_descriptions,
                             .num_vertex_buffers = 1,
                             .vertex_attributes = vert_attrs,
                             .num_vertex_attributes = 1},
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .rasterizer_state = {.fill_mode = SDL_GPU_FILLMODE_FILL,
                           .cull_mode = SDL_GPU_CULLMODE_NONE,
                           .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE},
      .target_info = target_info,
  };
  pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  m_fill_pipeline =
      SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
}

Renderer::~Renderer() {
  SDL_ReleaseGPUBuffer(m_GPU_device, mesh_vert_buffer);
  SDL_ReleaseGPUBuffer(m_GPU_device, mesh_index_buffer);
  SDL_ReleaseGPUGraphicsPipeline(m_GPU_device, m_fill_pipeline);
  destroyShader(m_base_vert_shader);
  destroyShader(m_base_frag_shader);
  SDL_WaitForGPUIdle(m_GPU_device);
  SDL_ReleaseWindowFromGPUDevice(m_GPU_device, m_window);
  SDL_DestroyGPUDevice(m_GPU_device);
  SDL_DestroyWindow(m_window);
  SDL_ShaderCross_Quit();
}

SDL_GPUDevice *Renderer::getGPUDevice() { return m_GPU_device; }

void Renderer::draw() {
  SDL_GPUColorTargetDescription color_target_descriptions[] = {
      {.format = SDL_GetGPUSwapchainTextureFormat(m_GPU_device, m_window)}};
  SDL_GPUGraphicsPipelineTargetInfo target_info = {
      .color_target_descriptions = color_target_descriptions,
      .num_color_targets = 1};
  // Create the pipelines
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = m_shader_table[m_base_vert_shader],
      .fragment_shader = m_shader_table[m_base_frag_shader],
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info = target_info,
  };
  pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  m_fill_pipeline =
      SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
  SDL_assert(m_fill_pipeline);
  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);

  SDL_GPUTexture *swapchain_texture;
  SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(
      command_buffer, m_window, &swapchain_texture, NULL, NULL));

  SDL_GPUColorTargetInfo color_target_infos[] = {
      {.texture = swapchain_texture,
       .mip_level = 0,
       .layer_or_depth_plane = 0,
       .clear_color = {.r = 0.878f, .g = 0.816f, .b = 1.0f},
       .load_op = SDL_GPU_LOADOP_CLEAR,
       .store_op = SDL_GPU_STOREOP_STORE,
       .resolve_texture = nullptr,
       .resolve_mip_level = 0,
       .resolve_layer = 0,
       .cycle = false,
       .cycle_resolve_texture = false}};

  SDL_assert(command_buffer);
  SDL_GPURenderPass *render_pass =
      SDL_BeginGPURenderPass(command_buffer, color_target_infos, 1, nullptr);

  SDL_BindGPUGraphicsPipeline(render_pass, m_fill_pipeline);

  SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
  SDL_EndGPURenderPass(render_pass);

  SDL_SubmitGPUCommandBuffer(command_buffer);
}

void Renderer::uploadMeshData() {
  if (mesh_vert_buffer != nullptr && mesh_index_buffer != nullptr) {
    return;
  }
  SDL_GPUBufferCreateInfo vert_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = sizeof(float) * 3 * 8};
  SDL_GPUBufferCreateInfo index_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = sizeof(uint16_t) * 36};
  mesh_vert_buffer = SDL_CreateGPUBuffer(m_GPU_device, &vert_buffer_info);
  mesh_index_buffer = SDL_CreateGPUBuffer(m_GPU_device, &index_buffer_info);
  SDL_GPUTransferBufferCreateInfo transfer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(float) * 3 * 8 + sizeof(uint16_t) * 36};
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);

  float *transferData = static_cast<float *>(
      SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer, false));
  transferData[0] = 0.0f;
  transferData[1] = 0.0f;
  transferData[2] = 0.0f;

  transferData[3] = 0.0f;
  transferData[4] = 10.0f;
  transferData[5] = 0.0f;

  transferData[6] = 10.0f;
  transferData[7] = 10.0f;
  transferData[8] = 0.0f;

  transferData[9] = 10.0f;
  transferData[10] = 0.0f;
  transferData[11] = 0.0f;

  transferData[12] = 0.0f;
  transferData[13] = 0.0f;
  transferData[14] = 10.0f;

  transferData[15] = 0.0f;
  transferData[16] = 10.0f;
  transferData[17] = 10.0f;

  transferData[18] = 10.0f;
  transferData[19] = 10.0f;
  transferData[20] = 10.0f;

  transferData[21] = 10.0f;
  transferData[22] = 0.0f;
  transferData[23] = 10.0f;

  uint16_t *indexData = reinterpret_cast<uint16_t *>(&transferData[24]);
  uint16_t indices[] = {2, 6, 7,

                        2, 7, 3,

                        0, 4, 5,

                        0, 5, 1,

                        6, 2, 1,

                        6, 1, 5,

                        3, 7, 5,

                        3, 4, 0,

                        7, 6, 5,

                        7, 5, 4,

                        2, 3, 0,

                        2, 0, 1};
  SDL_memcpy(indexData, indices, sizeof(indices));
  SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer);
  SDL_GPUCommandBuffer *uploadCmdBuf =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);
  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(uploadCmdBuf);
  SDL_GPUTransferBufferLocation vert_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = 0};
  SDL_GPUBufferRegion vert_region_location = {
      .buffer = mesh_vert_buffer, .offset = 0, .size = sizeof(float) * 3 * 8};
  SDL_UploadToGPUBuffer(copy_pass, &vert_transfer_location,
                        &vert_region_location, false);
  SDL_GPUTransferBufferLocation index_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = sizeof(float) * 3 * 8};
  SDL_GPUBufferRegion index_region_location = {
      .buffer = mesh_index_buffer, .offset = 0, .size = sizeof(uint16_t) * 36};
  SDL_UploadToGPUBuffer(copy_pass, &index_transfer_location,
                        &index_region_location, false);
  SDL_EndGPUCopyPass(copy_pass);
  SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
  SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
}

void Renderer::drawToTexture(Texture *tex) {
  SDL_assert(m_fill_pipeline);
  uploadMeshData();
  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);

  SDL_GPUColorTargetInfo color_target_infos[] = {
      {.texture = reinterpret_cast<SDL_GPUTexture *>(tex->opq_handle),
       .mip_level = 0,
       .layer_or_depth_plane = 0,
       .clear_color = {.r = 0.01f, .g = 0.01f, .b = 0.01f, .a = 1.0f},
       .load_op = SDL_GPU_LOADOP_CLEAR,
       .store_op = SDL_GPU_STOREOP_STORE,
       .resolve_texture = nullptr,
       .resolve_mip_level = 0,
       .resolve_layer = 0,
       .cycle = false,
       .cycle_resolve_texture = false}};

  SDL_assert(command_buffer);

  glm::mat4x4 MVP = generateViewProjMatrix(&camera);
  SDL_PushGPUVertexUniformData(command_buffer, 0, glm::value_ptr(MVP),
                               sizeof(float) * 16);
  SDL_GPURenderPass *render_pass =
      SDL_BeginGPURenderPass(command_buffer, color_target_infos, 1, nullptr);
  SDL_GPUBufferBinding vert_binding = {.buffer = mesh_vert_buffer, .offset = 0};
  SDL_GPUBufferBinding index_binding = {.buffer = mesh_index_buffer,
                                        .offset = 0};
  SDL_BindGPUVertexBuffers(render_pass, 0, &vert_binding, 1);
  SDL_BindGPUIndexBuffer(render_pass, &index_binding,
                         SDL_GPU_INDEXELEMENTSIZE_16BIT);
  SDL_BindGPUGraphicsPipeline(render_pass, m_fill_pipeline);

  SDL_DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0);
  SDL_EndGPURenderPass(render_pass);

  SDL_SubmitGPUCommandBuffer(command_buffer);
}

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

void Renderer::destroyShader(Shader &shader) {
  SDL_ReleaseGPUShader(m_GPU_device, m_shader_table[shader]);
  m_shader_table.erase(m_shader_table.begin() + shader);
  m_shader_type_record.erase(m_shader_type_record.begin() + shader);
  shader = -1;
}