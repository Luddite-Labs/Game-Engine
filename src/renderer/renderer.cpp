#include <cstring>
#include <renderer/renderer.hpp>

#include "glm/ext/matrix_clip_space.hpp"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <misc/log.hpp>

glm::mat4x4 generateProjMatrix(Camera *camera) {
  float radians = (camera->fov * SDL_PI_F) / 180.0f;
  float fovFactor = 1.0f / SDL_tanf(radians / 2.0f);
  float dist = camera->farPlane - camera->nearPlane;
  float lambda = (camera->farPlane / dist);
  glm::mat4x4 proj;
  if (camera->is_orthogonal) {
    proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f);
  } else {
    proj = glm::perspective(radians, camera->aspectRatio, camera->nearPlane,
                            camera->farPlane);
    // proj = glm::mat4x4(fovFactor / camera->aspectRatio, 0, 0, 0,

    //                    0, fovFactor, 0, 0,

    //                    0, 0, -lambda, -1,

    //                    0, 0, -lambda * camera->nearPlane, 0);
  }
  return proj;
}
glm::mat4x4 generateViewMatrix(Camera *camera) {
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
  return view;
}

Renderer::Renderer(SDL_Window *_window)
    : m_window(_window), m_create_info(), camera(),
      clear_color(0.0f, 0.0f, 0.0f, 1.0f), depth_buffer_texture() {
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
      .num_color_targets = 1,
      .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
      .has_depth_stencil_target = true,
  };
  // Create the pipelines
  SDL_GPUVertexAttribute vert_attrs[] = {
      {.location = 0,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
       .offset = 0},
      {.location = 1,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
       .offset = sizeof(glm::vec3)},
    {.location = 2,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
       .offset = sizeof(glm::vec2) + sizeof(glm::vec3)}};
  SDL_GPUVertexBufferDescription vert_buffer_descriptions[] = {
      {.slot = 0,
       .pitch = sizeof(Vertex),
       .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
       .instance_step_rate = 0}};
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = m_shader_table[m_base_vert_shader],
      .fragment_shader = m_shader_table[m_base_frag_shader],
      .vertex_input_state = {.vertex_buffer_descriptions =
                                 vert_buffer_descriptions,
                             .num_vertex_buffers = 1,
                             .vertex_attributes = vert_attrs,
                             .num_vertex_attributes = 3},
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .rasterizer_state = {.fill_mode = SDL_GPU_FILLMODE_FILL,
                           .cull_mode = SDL_GPU_CULLMODE_BACK,
                           .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE},
      .depth_stencil_state =
          {
              .compare_op = SDL_GPU_COMPAREOP_LESS,
              .write_mask = 0xFF,
              .enable_depth_test = true,
              .enable_depth_write = true,
              .enable_stencil_test = false,
          },
      .target_info = target_info,
  };
  m_fill_pipeline =
      SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
}

Renderer::~Renderer() {
  for (auto &mesh : m_mesh_table) {
    SDL_ReleaseGPUBuffer(m_GPU_device, mesh.vert_buffer);
    SDL_ReleaseGPUBuffer(m_GPU_device, mesh.index_buffer);
    mesh.vert_buffer = nullptr;
    mesh.index_buffer = nullptr;
  }
  destroyTexture(&depth_buffer_texture);
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

Mesh Renderer::createMesh(MeshData *mesh_data) {
  m_mesh_table.emplace_back();
  MeshInternal &mesh_internal = m_mesh_table.back();

  uint32_t vert_bytes =
      static_cast<uint32_t>(sizeof(Vertex) * mesh_data->vert_count);
  uint32_t index_bytes =
      static_cast<uint32_t>(sizeof(uint16_t) * mesh_data->index_count);

  SDL_GPUBufferCreateInfo vert_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vert_bytes};
  SDL_GPUBufferCreateInfo index_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = index_bytes};
  SDL_GPUBuffer *mesh_vert_buffer =
      SDL_CreateGPUBuffer(m_GPU_device, &vert_buffer_info);
  SDL_GPUBuffer *mesh_index_buffer =
      SDL_CreateGPUBuffer(m_GPU_device, &index_buffer_info);

  mesh_internal.vert_buffer = mesh_vert_buffer;
  mesh_internal.index_buffer = mesh_index_buffer;
  mesh_internal.vert_count = mesh_data->vert_count;
  mesh_internal.index_count = mesh_data->index_count;

  SDL_GPUTransferBufferCreateInfo transfer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = vert_bytes + index_bytes};
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);
  uint8_t *transfer_buffer_ptr = static_cast<uint8_t *>(
      SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer, false));

  SDL_memcpy(transfer_buffer_ptr, mesh_data->vert_buffer, vert_bytes);
  SDL_memcpy((transfer_buffer_ptr + vert_bytes), mesh_data->index_buffer,
             index_bytes);

  SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer);
  SDL_GPUCommandBuffer *uploadCmdBuf =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);
  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(uploadCmdBuf);
  SDL_GPUTransferBufferLocation vert_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = 0};
  SDL_GPUBufferRegion vert_region_location = {
      .buffer = mesh_vert_buffer, .offset = 0, .size = vert_bytes};
  SDL_UploadToGPUBuffer(copy_pass, &vert_transfer_location,
                        &vert_region_location, false);
  SDL_GPUTransferBufferLocation index_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = vert_bytes};
  SDL_GPUBufferRegion index_region_location = {
      .buffer = mesh_index_buffer, .offset = 0, .size = index_bytes};
  SDL_UploadToGPUBuffer(copy_pass, &index_transfer_location,
                        &index_region_location, false);
  SDL_EndGPUCopyPass(copy_pass);
  SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
  SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
  return m_mesh_table.size() - 1;
}

struct TransformMatrices {
  glm::mat4x4 view;
  glm::mat4x4 proj;
};

void Renderer::drawToTexture(Texture *tex) {
  SDL_assert(m_fill_pipeline);
  if (depth_buffer_texture.width != tex->width or
      depth_buffer_texture.height != tex->height) {
    const SDL_GPUTextureCreateInfo tex_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                 SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = tex->width,
        .height = tex->height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    SDL_GPUTexture *depth_texture =
        SDL_CreateGPUTexture(m_GPU_device, &tex_info);
    depth_buffer_texture.width = tex->width;
    depth_buffer_texture.height = tex->height;
    depth_buffer_texture.opq_handle =
        reinterpret_cast<uintptr_t>(depth_texture);
  }

  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);

  SDL_GPUColorTargetInfo color_target_infos[] = {
      {.texture = reinterpret_cast<SDL_GPUTexture *>(tex->opq_handle),
       .mip_level = 0,
       .layer_or_depth_plane = 0,
       .clear_color = {.r = clear_color.r,
                       .g = clear_color.g,
                       .b = clear_color.b,
                       .a = clear_color.a},
       .load_op = SDL_GPU_LOADOP_CLEAR,
       .store_op = SDL_GPU_STOREOP_STORE,
       .resolve_texture = nullptr,
       .resolve_mip_level = 0,
       .resolve_layer = 0,
       .cycle = false,
       .cycle_resolve_texture = false}};
  SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
      .texture =
          reinterpret_cast<SDL_GPUTexture *>(depth_buffer_texture.opq_handle),
      .clear_depth = 1,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
      .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
      .stencil_store_op = SDL_GPU_STOREOP_STORE,
      .cycle = true,
      .clear_stencil = 0,
  };
  SDL_assert(command_buffer);
  TransformMatrices transform_matrices;
  transform_matrices.view = generateViewMatrix(&camera);
  transform_matrices.proj = generateProjMatrix(&camera);
  SDL_PushGPUVertexUniformData(command_buffer, 0, &transform_matrices,
                               sizeof(transform_matrices));
  SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
      command_buffer, color_target_infos, 1, &depth_stencil_target_info);
  for (auto &mesh : m_mesh_table) {
    SDL_GPUBufferBinding vert_binding = {.buffer = mesh.vert_buffer,
                                         .offset = 0};
    SDL_GPUBufferBinding index_binding = {.buffer = mesh.index_buffer,
                                          .offset = 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vert_binding, 1);
    SDL_BindGPUIndexBuffer(render_pass, &index_binding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_BindGPUGraphicsPipeline(render_pass, m_fill_pipeline);

    SDL_DrawGPUIndexedPrimitives(render_pass, mesh.index_count, 1, 0, 0, 0);
  }
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
  auto shader_error = SDL_GetError();
  if (strlen(shader_error) != 0) {
    LOG_ERROR(shader_error);
  }
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

Texture Renderer::createTexture(uint32_t width, uint32_t height) {
  SDL_assert(m_window && m_GPU_device);
  const SDL_GPUTextureCreateInfo tex_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = width,
      .height = height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
      .props = 0};
  SDL_GPUTexture *tex = SDL_CreateGPUTexture(m_GPU_device, &tex_info);
  return Texture(tex, width, height);
}
void Renderer::destroyTexture(Texture *tex) {
  SDL_assert(m_window && m_GPU_device);
  SDL_ReleaseGPUTexture(m_GPU_device,
                        reinterpret_cast<SDL_GPUTexture *>(tex->opq_handle));
  tex->opq_handle = reinterpret_cast<uintptr_t>(nullptr);
}