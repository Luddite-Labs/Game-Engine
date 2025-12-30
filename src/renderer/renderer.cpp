#include <algorithm>
#include <filesystem>
#include <renderer/renderer.hpp>
#include <string>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "renderer/common.hpp"
#include "renderer/handles.hpp"
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <misc/log.hpp>

Renderer::Renderer() {
  clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
  SDL_assert(SDL_ShaderCross_Init());
  m_GPU_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  m_default_vert_shader = createShader(
      GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.hlsl", 0, 0, 0, 1);
  m_default_frag_shader = createShader(
      GAME_ENGINE_DEFAULT_SHADER_DIR "/base.frag.hlsl", 0, 0, 0, 0);

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
       .offset = offsetof(Vertex, normal)},
      {.location = 2,
       .buffer_slot = 0,
       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
       .offset = offsetof(Vertex, uv)}};
  SDL_GPUVertexBufferDescription vert_buffer_descriptions[] = {
      {.slot = 0,
       .pitch = sizeof(Vertex),
       .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
       .instance_step_rate = 0}};
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .vertex_shader = shader_storage.get(m_default_vert_shader).gpu_handle,
      .fragment_shader = shader_storage.get(m_default_frag_shader).gpu_handle,
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
  m_mesh_pipeline =
      SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
}

Renderer::~Renderer() {
  SDL_ReleaseGPUGraphicsPipeline(m_GPU_device, m_mesh_pipeline);
  destroyShader(m_default_vert_shader);
  destroyShader(m_default_frag_shader);
  SDL_WaitForGPUIdle(m_GPU_device);
  SDL_ReleaseWindowFromGPUDevice(m_GPU_device, m_window);
  SDL_DestroyGPUDevice(m_GPU_device);
  SDL_DestroyWindow(m_window);
  SDL_ShaderCross_Quit();
}

SDL_GPUDevice *Renderer::getGPUDevice() { return m_GPU_device; }

struct TransformMatrices {
  glm::mat4x4 view;
  glm::mat4x4 proj;
};

glm::mat4x4 getProjectionMatrix(const data::Camera &camera) {
  if (camera.is_orthogonal) {
    return glm::ortho(0.0f, camera.xmag, 0.0f, camera.ymag, camera.near_plane,
                      camera.far_plane);
  } else {
    return glm::perspective(camera.fov, camera.aspect_ratio, camera.near_plane,
                            camera.far_plane);
  }
}

//! infinite projection
void Renderer::drawToTexture(const handle::Texture &tex,
                             const handle::Camera &camera,
                             const glm::mat4x4 &camera_transform,
                             std::vector<DrawCommand> &draw_commands) {
  if (draw_commands.size() == 0) {
    return;
  }
  SDL_assert(m_mesh_pipeline);
  const auto &target_tex_data = texture_storage.get(tex);
  const auto &camera_data = camera_storage.get(camera);

  const SDL_GPUTextureCreateInfo depth_texture_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
               SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
      .width = target_tex_data.width,
      .height = target_tex_data.height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
  };
  SDL_GPUTexture *depth_texture =
      SDL_CreateGPUTexture(m_GPU_device, &depth_texture_info);

  std::sort(draw_commands.begin(), draw_commands.end(),
            [&](const auto &lhs, const auto &rhs) {
              if (lhs.material.slot_index < rhs.material.slot_index) {
                return lhs.mesh.slot_index < rhs.mesh.slot_index;
              }
              return lhs.material.slot_index < rhs.material.slot_index;
            });

  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);
  SDL_assert(command_buffer);
  SDL_GPUColorTargetInfo color_target_infos[] = {
      {.texture = target_tex_data.gpu_handle,
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
      .texture = depth_texture,
      .clear_depth = 1,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
      .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
      .stencil_store_op = SDL_GPU_STOREOP_STORE,
      .cycle = true,
      .clear_stencil = 0,
  };
  SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
      command_buffer, color_target_infos, 1, &depth_stencil_target_info);
  TransformMatrices transform_matrices;
  transform_matrices.proj = getProjectionMatrix(camera_data);
  for (int i = 0; i < draw_commands.size(); i++) {
    const auto &material =
        material_storage.get(draw_commands[i].mesh); // group mesh by material
    auto &color_texture = texture_storage.get(material.color);
    auto &color_sampler = sampler_storage.get(color_texture.sampler);
    const auto &mesh = mesh_storage.get(draw_commands[i].mesh);
    transform_matrices.view = camera_transform * draw_commands[i].transform;
    SDL_PushGPUVertexUniformData(command_buffer, 0, &transform_matrices,
                                 sizeof(transform_matrices));
    SDL_GPUBufferBinding vert_binding = {.buffer = mesh.vert_buffer,
                                         .offset = 0};
    SDL_GPUBufferBinding index_binding = {.buffer = mesh.index_buffer,
                                          .offset = 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &vert_binding, 1);
    SDL_BindGPUIndexBuffer(render_pass, &index_binding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding sampler_binding_info = {
        .texture = color_texture.gpu_handle,
        .sampler = color_sampler.gpu_handle};
    SDL_BindGPUFragmentSamplers(render_pass, 0, &sampler_binding_info, 1);
    SDL_BindGPUGraphicsPipeline(render_pass, m_mesh_pipeline);
    SDL_DrawGPUIndexedPrimitives(render_pass, mesh.index_count, 1, 0, 0, 0);
  }
  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);
  SDL_ReleaseGPUTexture(m_GPU_device, depth_texture);
}

//! deprecate once UI backend ready
uintptr_t Renderer::getTexture(handle::Texture texture) {
  return reinterpret_cast<uintptr_t>(texture_storage.get(texture).gpu_handle);
}

// Mesh
handle::Mesh Renderer::createMesh() { return mesh_storage.insert({}); }
void Renderer::refMesh(handle::Mesh mesh) { mesh_storage.ref(mesh); }
void Renderer::destroyMesh(handle::Mesh mesh) { mesh_storage.erase(mesh); }
void Renderer::uploadMeshBuffers(handle::Mesh mesh,
                                 std::shared_ptr<Vertex> vert_buffer,
                                 std::shared_ptr<uint16_t> index_buffer,
                                 uint32_t vert_count, uint32_t index_count) {
  //! release previous buffer
  data::Mesh &mesh_data = mesh_storage.get(mesh);
  mesh_data.index_count = index_count;
  mesh_data.vert_count = vert_count;

  uint32_t vert_bytes =
      static_cast<uint32_t>(sizeof(Vertex) * mesh_data.vert_count);
  uint32_t index_bytes =
      static_cast<uint32_t>(sizeof(uint16_t) * mesh_data.index_count);

  SDL_GPUBufferCreateInfo vert_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vert_bytes};
  SDL_GPUBufferCreateInfo index_buffer_info = {
      .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = index_bytes};
  mesh_data.vert_buffer = SDL_CreateGPUBuffer(m_GPU_device, &vert_buffer_info);
  mesh_data.index_buffer =
      SDL_CreateGPUBuffer(m_GPU_device, &index_buffer_info);

  SDL_GPUTransferBufferCreateInfo transfer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = vert_bytes + index_bytes};
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);
  uint8_t *transfer_buffer_ptr = static_cast<uint8_t *>(
      SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer, false));

  SDL_memcpy(transfer_buffer_ptr, vert_buffer.get(), vert_bytes);
  SDL_memcpy((transfer_buffer_ptr + vert_bytes), index_buffer.get(),
             index_bytes);

  SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer);
  SDL_GPUCommandBuffer *command_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);
  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
  SDL_GPUTransferBufferLocation vert_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = 0};
  SDL_GPUBufferRegion vert_region_location = {
      .buffer = mesh_data.vert_buffer, .offset = 0, .size = vert_bytes};
  SDL_UploadToGPUBuffer(copy_pass, &vert_transfer_location,
                        &vert_region_location, false);
  SDL_GPUTransferBufferLocation index_transfer_location = {
      .transfer_buffer = transfer_buffer, .offset = vert_bytes};
  SDL_GPUBufferRegion index_region_location = {
      .buffer = mesh_data.index_buffer, .offset = 0, .size = index_bytes};
  SDL_UploadToGPUBuffer(copy_pass, &index_transfer_location,
                        &index_region_location, false);
  SDL_EndGPUCopyPass(copy_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);
  SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
}
AABB Renderer::getAABB(handle::Mesh mesh) {
  return mesh_storage.get(mesh).aabb;
}
void Renderer::setAABB(handle::Mesh mesh, AABB aabb) {
  mesh_storage.get(mesh).aabb = aabb;
  mesh_storage.setIsEdited(mesh);
}

// Material
handle::Material Renderer::createMaterial() {
  return material_storage.insert({});
}
void Renderer::refMaterial(handle::Material material) {
  material_storage.ref(material);
}
void Renderer::destroyMaterial(handle::Material material) {
  material_storage.erase(material);
}
glm::vec4 Renderer::getMaterialColorFactor(handle::Material material) {
  return material_storage.get(material).color_factor;
}
glm::vec3 Renderer::getMaterialEmissiveFactor(handle::Material material) {
  return material_storage.get(material).emissive_factor;
}
handle::Texture Renderer::getMaterialNormalTexture(handle::Material material) {
  return material_storage.get(material).normal;
}
handle::Texture
Renderer::getMaterialEmissiveTexture(handle::Material material) {
  return material_storage.get(material).emissive;
}
handle::Texture
Renderer::getMaterialOcclusionTexture(handle::Material material) {
  return material_storage.get(material).occlusion;
}
handle::Texture Renderer::getMaterialColorTexture(handle::Material material) {
  return material_storage.get(material).color;
}
handle::Texture
Renderer::getMaterialMetallicRoughness(handle::Material material) {
  return material_storage.get(material).metallic_roughness;
}
float Renderer::getMaterialNormalScale(handle::Material material) {
  return material_storage.get(material).normal_scale;
}
float Renderer::getMaterialMetallicFactor(handle::Material material) {
  return material_storage.get(material).metallic_factor;
}
float Renderer::getMaterialRoughnessFactor(handle::Material material) {
  return material_storage.get(material).metallic_factor;
}
void Renderer::setMaterialColorFactor(handle::Material material,
                                      glm::vec4 color_factor) {
  material_storage.get(material).color_factor = color_factor;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialEmissiveFactor(handle::Material material,
                                         glm::vec3 emissive_factor) {
  material_storage.get(material).emissive_factor = emissive_factor;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialNormalTexture(handle::Material material,
                                        handle::Texture normal) {
  texture_storage.ref(
      normal); //! erase previous texture reserve index 0 for invalid in slotmap
  material_storage.get(material).normal = normal;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialEmissiveTexture(handle::Material material,
                                          handle::Texture emissive) {
  texture_storage.ref(emissive);
  material_storage.get(material).emissive = emissive;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialOcclusionTexture(handle::Material material,
                                           handle::Texture occlusion) {
  texture_storage.ref(occlusion);
  material_storage.get(material).occlusion = occlusion;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialColorTexture(handle::Material material,
                                       handle::Texture color) {
  texture_storage.ref(color);
  material_storage.get(material).color = color;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialMetallicRoughness(
    handle::Material material, handle::Texture metallic_roughness) {
  texture_storage.ref(metallic_roughness);
  material_storage.get(material).metallic_roughness = metallic_roughness;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialNormalScale(handle::Material material,
                                      float normal_scale) {
  material_storage.get(material).normal_scale = normal_scale;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialMetallicFactor(handle::Material material,
                                         float metallic_factor) {
  material_storage.get(material).metallic_factor = metallic_factor;
  material_storage.setIsEdited(material);
}
void Renderer::setMaterialRoughnessFactor(handle::Material material,
                                          float roughness_factor) {
  material_storage.get(material).metallic_factor = roughness_factor;
  material_storage.setIsEdited(material);
}

// Texture
handle::Texture Renderer::createTexture(uint32_t width, uint32_t height,
                                        TextureUsageFlags usage_flags,
                                        TextureFormat format) {
  SDL_assert(m_GPU_device);
  const SDL_GPUTextureCreateInfo tex_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = static_cast<SDL_GPUTextureFormat>(format),
      .usage = static_cast<SDL_GPUTextureUsageFlags>(usage_flags),
      .width = width,
      .height = height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
      .props = 0};
  SDL_GPUTexture *tex_data = SDL_CreateGPUTexture(m_GPU_device, &tex_info);
  return texture_storage.insert({
      .gpu_handle = tex_data,
      .width = width,
      .height = height,
      .format = format,
  });
}
void Renderer::refTexture(handle::Texture texture) {
  texture_storage.ref(texture);
}
void Renderer::destroyTexture(handle::Texture texture) {
  texture_storage.erase(texture);
}
uint32_t Renderer::getTextureWidth(handle::Texture texture) {
  return texture_storage.get(texture).width;
}
uint32_t Renderer::getTextureHeight(handle::Texture texture) {
  return texture_storage.get(texture).height;
}
TextureFormat Renderer::getTextureFormat(handle::Texture texture) {
  return texture_storage.get(texture).format;
}
uint32_t Renderer::getTextureUsageFlags(handle::Texture texture) {
  return texture_storage.get(texture).usage_flags;
}
handle::Sampler Renderer::getTextureSampler(handle::Texture texture) {
  return texture_storage.get(texture).sampler;
}
void Renderer::setTextureWidth(handle::Texture texture, uint32_t width) {
  texture_storage.get(texture).width = width;
  texture_storage.setIsEdited(texture);
}
void Renderer::setTextureHeight(handle::Texture texture, uint32_t height) {
  texture_storage.get(texture).height = height;
  texture_storage.setIsEdited(texture);
}
void Renderer::setTextureFormat(handle::Texture texture, TextureFormat format) {
  texture_storage.get(texture).format = format;
  texture_storage.setIsEdited(texture);
}
void Renderer::setTextureUsageFlags(handle::Texture texture,
                                    uint32_t usage_flags) {
  texture_storage.get(texture).usage_flags = usage_flags;
  texture_storage.setIsEdited(texture);
}
void Renderer::setTextureSampler(handle::Texture texture,
                                 handle::Sampler sampler) {
  texture_storage.get(texture).sampler = sampler;
  sampler_storage.ref(sampler);
  // texture_storage.setIsEdited(texture); //! no change texture state so no
  // refresh needed?
}
void Renderer::uploadBufferToTexture(handle::Texture texture,
                                     std::shared_ptr<uint8_t> buffer,
                                     size_t offset, size_t count) {
  auto &tex = texture_storage.get(texture);
  SDL_GPUTransferBufferCreateInfo transfer_create_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = static_cast<uint32_t>(count)};
  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);
  uint8_t *transfer_buffer_ptr = static_cast<uint8_t *>(
      SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer, false));

  SDL_memcpy(transfer_buffer_ptr, buffer.get(), count);

  SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer);
  SDL_GPUCommandBuffer *copy_cmd_buffer =
      SDL_AcquireGPUCommandBuffer(m_GPU_device);
  SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buffer);
  SDL_GPUTextureTransferInfo tex_tranfer_location = {
      .transfer_buffer = transfer_buffer, .offset = 0};
  SDL_GPUTextureRegion tex_transfer_region = {
      .texture = tex.gpu_handle, .w = tex.width, .h = tex.height, .d = 1};
  SDL_UploadToGPUTexture(copy_pass, &tex_tranfer_location, &tex_transfer_region,
                         false);
  SDL_EndGPUCopyPass(copy_pass);
  SDL_SubmitGPUCommandBuffer(copy_cmd_buffer);
  SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
}

// Shader
handle::Shader Renderer::createShader(const std::string &shader_file_path,
                                      uint32_t num_samplers,
                                      uint32_t num_storage_textures,
                                      uint32_t num_storage_buffers,
                                      uint32_t num_uniform_buffers) {
  SDL_assert(m_GPU_device != nullptr);
  std::filesystem::path shader_fs_path(shader_file_path);
  std::string stem = std::string(shader_fs_path.stem());
  auto cached_shader_file_path = stem + ".spirv";
  bool is_cached = SDL_GetPathInfo(cached_shader_file_path.c_str(), NULL);
  std::string target_file_path;
  if (is_cached) {
    target_file_path = cached_shader_file_path;
  } else {
    SDL_assert(SDL_GetPathInfo(shader_file_path.c_str(), NULL));
    target_file_path = shader_file_path;
  }

  ShaderType shader_type;
  if (stem.find(".vert") != std::string::npos) {
    shader_type = ShaderType::VERTEX;
  } else if (stem.find(".frag") != std::string::npos) {
    shader_type = ShaderType::FRAGMENT;
  } else {
    SDL_assert(false);
  }

  data::Shader shader_data = {.num_samplers = num_samplers,
                              .num_storage_textures = num_storage_textures,
                              .num_storage_buffers = num_storage_buffers,
                              .num_uniform_buffers = num_uniform_buffers,
                              .type = shader_type,
                              .gpu_handle = nullptr};

  size_t data_size;
  uint8_t *buffer = static_cast<uint8_t *>(
      SDL_LoadFile(target_file_path.c_str(), &data_size));
  if (is_cached) {
    SDL_GPUShaderCreateInfo create_info = {
        .code_size = data_size,
        .code = buffer,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = static_cast<SDL_GPUShaderStage>(shader_type),
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers,
        .props = 0};
    shader_data.gpu_handle = SDL_CreateGPUShader(m_GPU_device, &create_info);
  } else {
    SDL_ShaderCross_HLSL_Info hlsl_info = {
        .source = reinterpret_cast<char *>(buffer),
        .entrypoint = "main",
        .include_dir = nullptr, //! add option for includes later
        .defines = nullptr,
        .shader_stage = static_cast<SDL_ShaderCross_ShaderStage>(shader_type),
        .enable_debug = true,
        .name = nullptr,
    };
    SDL_ShaderCross_GraphicsShaderMetadata shadercross_graphics_metadata = {
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers};

    shader_data.gpu_handle = SDL_ShaderCross_CompileGraphicsShaderFromHLSL(
        m_GPU_device, &hlsl_info, &shadercross_graphics_metadata);
    auto shader_error = SDL_GetError();
    if (strlen(shader_error) != 0) {
      LOG_ERROR(shader_error);
    }
    SDL_assert(shader_data.gpu_handle != nullptr);
  }
  return shader_storage.insert(shader_data);
}
void Renderer::refShader(handle::Shader shader) { shader_storage.ref(shader); }
void Renderer::destroyShader(handle::Shader shader) {
  shader_storage.erase(shader);
}
ShaderType Renderer::getShaderType(handle::Shader shader) {
  return shader_storage.get(shader).type;
}
// const char *getShaderFilePath(handle::Shader shader);
uint32_t Renderer::getShaderNumSamplers(handle::Shader shader) {
  return shader_storage.get(shader).num_samplers;
}
uint32_t Renderer::getShaderNumStorageTextures(handle::Shader shader) {
  return shader_storage.get(shader).num_storage_textures;
}
uint32_t Renderer::getShaderNumStorageBuffers(handle::Shader shader) {
  return shader_storage.get(shader).num_storage_buffers;
}
uint32_t Renderer::getShaderNumUniformBuffers(handle::Shader shader) {
  return shader_storage.get(shader).num_uniform_buffers;
}

// Sampler
handle::Sampler Renderer::createSampler(SamplerFilteringModes mag_filter,
                                        SamplerFilteringModes min_filter,
                                        SamplerAddressingModes u_addressing,
                                        SamplerAddressingModes v_addressing) {
  SDL_assert(m_GPU_device != nullptr);
  data::Sampler sampler_data = {.mag_filter = mag_filter,
                                .min_filter = min_filter,
                                .u_addressing = u_addressing,
                                .v_addressing = v_addressing,
                                .gpu_handle = nullptr};
  SDL_GPUSamplerCreateInfo sampler_info = {
      .min_filter = static_cast<SDL_GPUFilter>(sampler_data.min_filter),
      .mag_filter = static_cast<SDL_GPUFilter>(sampler_data.mag_filter),
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      .address_mode_u =
          static_cast<SDL_GPUSamplerAddressMode>(sampler_data.u_addressing),
      .address_mode_v =
          static_cast<SDL_GPUSamplerAddressMode>(sampler_data.v_addressing),
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  sampler_data.gpu_handle = SDL_CreateGPUSampler(m_GPU_device, &sampler_info);
  return sampler_storage.insert(sampler_data);
}
void Renderer::refSampler(handle::Sampler sampler) {
  sampler_storage.ref(sampler);
}
void Renderer::destroySampler(handle::Sampler sampler) {
  sampler_storage.erase(sampler);
}
SamplerFilteringModes Renderer::getSamplerMagFilter(handle::Sampler sampler) {
  return sampler_storage.get(sampler).mag_filter;
}
SamplerFilteringModes Renderer::getSamplerMinFilter(handle::Sampler sampler) {
  return sampler_storage.get(sampler).min_filter;
}
SamplerAddressingModes
Renderer::getSamplerUAddressing(handle::Sampler sampler) {
  return sampler_storage.get(sampler).u_addressing;
}
SamplerAddressingModes
Renderer::getSamplerVAddressing(handle::Sampler sampler) {
  return sampler_storage.get(sampler).v_addressing;
}
void Renderer::setSamplerMagFilter(handle::Sampler sampler,
                                   SamplerFilteringModes mode) {
  sampler_storage.get(sampler).mag_filter = mode;
  sampler_storage.setIsEdited(sampler);
}
void Renderer::setSamplerMinFilter(handle::Sampler sampler,
                                   SamplerFilteringModes mode) {
  sampler_storage.get(sampler).min_filter = mode;
  sampler_storage.setIsEdited(sampler);
}
void Renderer::setSamplerUAddressing(handle::Sampler sampler,
                                     SamplerAddressingModes mode) {
  sampler_storage.get(sampler).u_addressing = mode;
  sampler_storage.setIsEdited(sampler);
}
void Renderer::setSamplerVAddressing(handle::Sampler sampler,
                                     SamplerAddressingModes mode) {
  sampler_storage.get(sampler).v_addressing = mode;
  sampler_storage.setIsEdited(sampler);
}

// Camera
handle::Camera Renderer::createCamera() {
  handle::Camera camera = camera_storage.insert({});
  setPerspectiveCamera(camera, 1.77);
  return camera;
}
void Renderer::refCamera(handle::Camera camera) { camera_storage.ref(camera); }
void Renderer::destroyCamera(handle::Camera camera) {
  camera_storage.erase(camera);
}
void Renderer::setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
                                    float fov, float near_plane,
                                    float far_plane) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.aspect_ratio = aspect_ratio;
  camera_data.fov = fov;
  camera_data.near_plane = near_plane;
  camera_data.far_plane = far_plane;
  camera_data.is_orthogonal = false;
  camera_storage.setIsEdited(camera);
}
void Renderer::setOrthogonalCamera(handle::Camera camera, float xmag,
                                   float ymag, float near_plane,
                                   float far_plane) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.xmag = xmag;
  camera_data.ymag = ymag;
  camera_data.near_plane = near_plane;
  camera_data.far_plane = far_plane;
  camera_data.is_orthogonal = true;
  camera_storage.setIsEdited(camera);
}
float Renderer::getCameraAspectRatio(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.aspect_ratio;
}
float Renderer::getCameraFOV(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.fov;
}
float Renderer::getCameraXMag(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.xmag;
}
float Renderer::getCameraYMag(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.ymag;
}
float Renderer::getCameraNearPlane(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.near_plane;
}
float Renderer::getCameraFarPlane(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.far_plane;
}
bool Renderer::isCameraOrthogonal(handle::Camera camera) {
  data::Camera camera_data = camera_storage.get(camera);
  return camera_data.is_orthogonal;
}
void Renderer::setCameraAspectRatio(handle::Camera camera, float aspect_ratio) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.aspect_ratio = aspect_ratio;
  camera_storage.setIsEdited(camera);
}
void Renderer::setCameraFOV(handle::Camera camera, float fov) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.fov = fov;
  camera_storage.setIsEdited(camera);
}
void Renderer::setCameraXMag(handle::Camera camera, float xmag) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.xmag = xmag;
  camera_storage.setIsEdited(camera);
}
void Renderer::setCameraYMag(handle::Camera camera, float ymag) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.ymag = ymag;
  camera_storage.setIsEdited(camera);
}
void Renderer::setCameraNearPlane(handle::Camera camera, float near_plane) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.near_plane = near_plane;
  camera_storage.setIsEdited(camera);
}
void Renderer::setCameraFarPlane(handle::Camera camera, float far_plane) {
  data::Camera &camera_data = camera_storage.get(camera);
  camera_data.far_plane = far_plane;
  camera_storage.setIsEdited(camera);
}