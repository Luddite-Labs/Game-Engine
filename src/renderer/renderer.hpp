#pragma once
#include "SDL3/SDL_gpu.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <renderer/common.hpp>
#include <renderer/handles.hpp>
#include <renderer/stores.hpp>

#include <misc/slot-map.hpp>
#include <misc/utils.hpp>
#include <soa_vector.hpp>

struct DrawCommand {
  handle::Material material;
  handle::Mesh mesh;
  glm::mat4x4 transform;
};

class Renderer {
public:
  glm::vec4 clear_color;

private:
  inline static Renderer *singleton = nullptr;

  SDL_Window *m_window;
  SDL_GPUDevice *m_GPU_device;

  SDL_GPUGraphicsPipeline *m_mesh_pipeline;

  handle::Texture m_depth;
  handle::Sampler m_default_sample;
  handle::Shader m_default_vert_shader;
  handle::Shader m_default_frag_shader;

  SlotMap<std::vector<data::Camera>, data::Camera> camera_storage;
  SlotMap<std::vector<data::Sampler>, data::Sampler> sampler_storage;
  SlotMap<std::vector<data::Shader>, data::Shader> shader_storage;
  SlotMap<std::vector<data::Texture>, data::Texture> texture_storage;
  SlotMap<std::vector<data::Material>, data::Material> material_storage;
  SlotMap<std::vector<data::Mesh>, data::Mesh> mesh_storage;

  void uploadMeshData();

  Renderer();
  ~Renderer();

public:
  static void init() { singleton = new Renderer(); }
  static void destroy() { delete singleton; }
  static Renderer *getSingleton() {
#ifdef GAME_ENGINE_DEBUG_MODE
    SDL_assert(singleton != nullptr);
#endif
    return singleton;
  }

  SDL_GPUDevice *getGPUDevice();
  void drawToTexture(const handle::Texture &tex, const handle::Camera &camera,
                     const glm::mat4x4 &camera_transform,
                     std::vector<DrawCommand> &draw_commands);

  handle::Shader createShader(const std::string &shader_file_path,
                              uint32_t num_samplers = 0,
                              uint32_t num_storage_textures = 0,
                              uint32_t num_storage_buffers = 0,
                              uint32_t num_uniform_buffers = 0);
  void refShader(handle::Shader shader);
  void destroyShader(handle::Shader shader);
  // const char *getShaderFilePath(handle::Shader shader);
  uint32_t getShaderNumSamplers(handle::Shader shader);
  uint32_t getShaderNumStorageTextures(handle::Shader shader);
  uint32_t getShaderNumStorageBuffers(handle::Shader shader);
  uint32_t getShaderNumUniformBuffers(handle::Shader shader);
  ShaderType getShaderType(handle::Shader shader);

  // Camera
  handle::Camera createCamera();
  void refCamera(handle::Camera camera);
  void destroyCamera(handle::Camera camera);
  void setPerspectiveCamera(handle::Camera camera, float aspect_ratio,
                            float fov = glm::radians(75.0f),
                            float near_plane = 1.0f, float far_plane = 1000.0f);
  void setOrthogonalCamera(handle::Camera camera, float xmag = 50.0f,
                           float ymag = 50.0f, float near_plane = 10.0f,
                           float far_plane = 100.0f);
  float getCameraAspectRatio(handle::Camera camera);
  float getCameraFOV(handle::Camera camera);
  float getCameraXMag(handle::Camera camera);
  float getCameraYMag(handle::Camera camera);
  float getCameraNearPlane(handle::Camera camera);
  float getCameraFarPlane(handle::Camera camera);
  bool isCameraOrthogonal(handle::Camera camera);
  void setCameraAspectRatio(handle::Camera camera, float aspect_ratio);
  void setCameraFOV(handle::Camera camera, float fov);
  void setCameraXMag(handle::Camera camera, float xmag);
  void setCameraYMag(handle::Camera camera, float ymag);
  void setCameraNearPlane(handle::Camera camera, float near_plane);
  void setCameraFarPlane(handle::Camera camera, float far_plane);

  handle::Material createMaterial();
  void refMaterial(handle::Material material);
  void destroyMaterial(handle::Material material);
  glm::vec4 getMaterialColorFactor(handle::Material material);
  glm::vec3 getMaterialEmissiveFactor(handle::Material material);
  handle::Texture getMaterialNormalTexture(handle::Material material);
  handle::Texture getMaterialEmissiveTexture(handle::Material material);
  handle::Texture getMaterialOcclusionTexture(handle::Material material);
  handle::Texture getMaterialColorTexture(handle::Material material);
  handle::Texture getMaterialMetallicRoughness(handle::Material material);
  float getMaterialNormalScale(handle::Material material);
  float getMaterialMetallicFactor(handle::Material material);
  float getMaterialRoughnessFactor(handle::Material material);
  void setMaterialColorFactor(handle::Material material,
                              glm::vec4 color_factor);
  void setMaterialEmissiveFactor(handle::Material material,
                                 glm::vec3 emissive_factor);
  void setMaterialNormalTexture(handle::Material material,
                                handle::Texture normal);
  void setMaterialEmissiveTexture(handle::Material material,
                                  handle::Texture emissive);
  void setMaterialOcclusionTexture(handle::Material material,
                                   handle::Texture occlusion);
  void setMaterialColorTexture(handle::Material material,
                               handle::Texture color);
  void setMaterialMetallicRoughness(handle::Material material,
                                    handle::Texture metallic_roughness);
  void setMaterialNormalScale(handle::Material material, float normal_scale);
  void setMaterialMetallicFactor(handle::Material material,
                                 float metallic_factor);
  void setMaterialRoughnessFactor(handle::Material material,
                                  float roughness_factor);

  uintptr_t getTexture(handle::Texture texture);
  // any param change will rebuild texture
  handle::Texture
  createTexture(uint32_t width, uint32_t height,
                TextureUsageFlags usage_flags = TextureUsageFlags::SAMPLER,
                TextureFormat format =
                    TextureFormat::R8G8B8A8_UNORM); // creates an entry that
  // needs data to be uploaded
  void refTexture(handle::Texture texture);
  void destroyTexture(handle::Texture texture);
  uint32_t getTextureWidth(handle::Texture texture);
  uint32_t getTextureHeight(handle::Texture texture);
  TextureFormat getTextureFormat(handle::Texture texture);
  uint32_t getTextureUsageFlags(handle::Texture texture);
  handle::Sampler getTextureSampler(handle::Texture texture);
  void setTextureWidth(handle::Texture texture, uint32_t width);
  void setTextureHeight(handle::Texture texture, uint32_t height);
  void setTextureFormat(handle::Texture texture, TextureFormat format);
  void setTextureUsageFlags(handle::Texture texture, uint32_t usage_flags);
  void uploadBufferToTexture(handle::Texture texture,
                             std::shared_ptr<uint8_t> buffer, size_t offset,
                             size_t count);
  void setTextureSampler(handle::Texture texture, handle::Sampler sampler);

  // Sampler
  handle::Sampler createSampler(
      SamplerFilteringModes mag_filter = SamplerFilteringModes::LINEAR,
      SamplerFilteringModes min_filter = SamplerFilteringModes::LINEAR,
      SamplerAddressingModes u_addressing =
          SamplerAddressingModes::CLAMP_TO_EDGE,
      SamplerAddressingModes v_addressing =
          SamplerAddressingModes::CLAMP_TO_EDGE);
  void refSampler(handle::Sampler sampler);
  void destroySampler(handle::Sampler sampler);
  SamplerFilteringModes getSamplerMagFilter(handle::Sampler sampler);
  SamplerFilteringModes getSamplerMinFilter(handle::Sampler sampler);
  SamplerAddressingModes getSamplerUAddressing(handle::Sampler sampler);
  SamplerAddressingModes getSamplerVAddressing(handle::Sampler sampler);
  void setSamplerMagFilter(handle::Sampler sampler, SamplerFilteringModes mode);
  void setSamplerMinFilter(handle::Sampler sampler, SamplerFilteringModes mode);
  void setSamplerUAddressing(handle::Sampler sampler,
                             SamplerAddressingModes mode);
  void setSamplerVAddressing(handle::Sampler sampler,
                             SamplerAddressingModes mode);

  handle::Mesh createMesh();
  void refMesh(handle::Mesh mesh);
  void destroyMesh(handle::Mesh mesh);
  void uploadMeshBuffers(handle::Mesh mesh, std::shared_ptr<Vertex> vert_buffer,
                         std::shared_ptr<uint16_t> index_buffer,
                         uint32_t vert_count, uint32_t index_count);
  AABB getAABB(handle::Mesh mesh);
  void setAABB(handle::Mesh mesh, AABB aabb);
};