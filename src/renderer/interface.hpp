#pragma once

#include "SDL3/SDL_gpu.h"
#include "glm/ext/matrix_float4x4.hpp"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <renderer/common.hpp>
#include <renderer/handles.hpp>
#include <renderer/renderer.hpp>
#include <variant>

/**
 * Handle based architecture all real code and memory storage in the
 * rendering system and use these classes that contain handles to
 * interface in a normal OOP way to the data
 */
// add copy functions
namespace interface {
class OrthogonalCamera {
private:
  handle::Camera handle;

public:
  OrthogonalCamera() {
    handle = Renderer::getSingleton()->createCamera();
    Renderer::getSingleton()->setOrthogonalCamera(handle);
  }
  OrthogonalCamera(const OrthogonalCamera &other) {
    handle = other.handle;
    Renderer::getSingleton()->refCamera(handle);
  }
  ~OrthogonalCamera() { Renderer::getSingleton()->destroyCamera(handle); }
  float getXMag() { return Renderer::getSingleton()->getCameraXMag(handle); }
  float getYMag() { return Renderer::getSingleton()->getCameraYMag(handle); }
  float getNearPlane() {
    return Renderer::getSingleton()->getCameraNearPlane(handle);
  }
  float getFarPlane() {
    return Renderer::getSingleton()->getCameraFarPlane(handle);
  }
  void setCameraXMag(float xmag) {
    Renderer::getSingleton()->setCameraXMag(handle, xmag);
  }
  void setCameraYMag(float ymag) {
    Renderer::getSingleton()->setCameraYMag(handle, ymag);
  }
  void setCameraNearPlane(float near_plane) {
    Renderer::getSingleton()->setCameraNearPlane(handle, near_plane);
  }
  void setCameraFarPlane(float far_plane) {
    Renderer::getSingleton()->setCameraFarPlane(handle, far_plane);
  }
  friend class Renderer;
};

class PerspectiveCamera {
private:
  handle::Camera handle;

public:
  PerspectiveCamera(float aspect_ratio) {
    handle = Renderer::getSingleton()->createCamera();
    Renderer::getSingleton()->setPerspectiveCamera(handle, aspect_ratio);
  }
  PerspectiveCamera(const PerspectiveCamera &other) {
    handle = other.handle;
    Renderer::getSingleton()->refCamera(handle);
  }
  ~PerspectiveCamera() { Renderer::getSingleton()->destroyCamera(handle); }
  float getAspectRatio() {
    return Renderer::getSingleton()->getCameraAspectRatio(handle);
  }
  float getFOV() { return Renderer::getSingleton()->getCameraFOV(handle); }
  float getNearPlane() {
    return Renderer::getSingleton()->getCameraNearPlane(handle);
  }
  float getFarPlane() {
    return Renderer::getSingleton()->getCameraFarPlane(handle);
  }
  void setCameraAspectRatio(float aspect_ratio) {
    Renderer::getSingleton()->setCameraAspectRatio(handle, aspect_ratio);
  }
  void setCameraFOV(float fov) {
    Renderer::getSingleton()->setCameraFOV(handle, fov);
  }
  void setCameraNearPlane(float near_plane) {
    Renderer::getSingleton()->setCameraNearPlane(handle, near_plane);
  }
  void setCameraFarPlane(float far_plane) {
    Renderer::getSingleton()->setCameraFarPlane(handle, far_plane);
  }
  friend class Renderer;
};

struct MeshData {
public:
  // vertex position and normal interleaved data expected both float3
  void *vert_buffer;
  void *index_buffer;
  uint32_t vert_count;
  uint32_t index_count;
  AABB aabb;
};

struct Sampler {
private:
  handle::Sampler handle;
  Sampler(handle::Sampler handle) : handle(handle) {}

public:
  Sampler(SamplerFilteringModes mag_filter = SamplerFilteringModes::LINEAR,
          SamplerFilteringModes min_filter = SamplerFilteringModes::LINEAR,
          SamplerAddressingModes u_addressing =
              SamplerAddressingModes::CLAMP_TO_EDGE,
          SamplerAddressingModes v_addressing =
              SamplerAddressingModes::CLAMP_TO_EDGE) {
    handle = Renderer::getSingleton()->createSampler(
        mag_filter, min_filter, u_addressing, v_addressing);
  }

  Sampler(const Sampler &other) {
    handle = other.handle;
    Renderer::getSingleton()->refSampler(handle);
  }
  ~Sampler() { Renderer::getSingleton()->destroySampler(handle); }

  SamplerFilteringModes getMagFilter() {
    return Renderer::getSingleton()->getSamplerMagFilter(handle);
  }
  SamplerFilteringModes getMinFilter() {
    return Renderer::getSingleton()->getSamplerMinFilter(handle);
  }
  SamplerAddressingModes getUAddressing() {
    return Renderer::getSingleton()->getSamplerUAddressing(handle);
  }
  SamplerAddressingModes getVAddressing() {
    return Renderer::getSingleton()->getSamplerVAddressing(handle);
  }
  void setMagFilter(SamplerFilteringModes mode) {
    Renderer::getSingleton()->setSamplerMagFilter(handle, mode);
  }
  void setMinFilter(SamplerFilteringModes mode) {
    Renderer::getSingleton()->setSamplerMinFilter(handle, mode);
  }
  void setUAddressing(SamplerAddressingModes mode) {
    Renderer::getSingleton()->setSamplerUAddressing(handle, mode);
  }
  void setVAddressing(SamplerAddressingModes mode) {
    Renderer::getSingleton()->setSamplerVAddressing(handle, mode);
  }
  friend class Texture;
};

struct Texture {
private:
  handle::Texture handle;

  Texture(handle::Texture handle) : handle(handle) {}

public:
  Texture(uint32_t width, uint32_t height,
          TextureUsageFlags usage_flags = TextureUsageFlags::SAMPLER,
          TextureFormat format = TextureFormat::R8G8B8A8_UNORM) {
    handle = Renderer::getSingleton()->createTexture(width, height, usage_flags,
                                                     format);
  }
  Texture(const Texture &other) {
    handle = other.handle;
    Renderer::getSingleton()->refTexture(handle);
  }
  ~Texture() { Renderer::getSingleton()->destroyTexture(handle); }
  uint32_t getWidth() {
    return Renderer::getSingleton()->getTextureWidth(handle);
  }
  uint32_t getHeight() {
    return Renderer::getSingleton()->getTextureHeight(handle);
  }
  TextureFormat getFormat() {
    return Renderer::getSingleton()->getTextureFormat(handle);
  }
  uint32_t getUsageFlags() {
    return Renderer::getSingleton()->getTextureUsageFlags(handle);
  }
  interface::Sampler getSampler() {
    handle::Sampler sampler =
        Renderer::getSingleton()->getTextureSampler(handle);
    Renderer::getSingleton()->refSampler(sampler);
    return {sampler};
  }
  void setWidth(uint32_t width) {
    Renderer::getSingleton()->setTextureWidth(handle, width);
  }
  void setHeight(uint32_t height) {
    Renderer::getSingleton()->setTextureHeight(handle, height);
  }
  void setFormat(TextureFormat format) {
    Renderer::getSingleton()->setTextureFormat(handle, format);
  }
  void setUsageFlags(uint32_t usage_flags) {
    Renderer::getSingleton()->setTextureUsageFlags(handle, usage_flags);
  }
  void uploadBuffer(std::shared_ptr<uint8_t> buffer, size_t offset,
                    size_t count) {
    Renderer::getSingleton()->uploadBufferToTexture(handle, buffer, offset,
                                                    count);
  }
  void setSampler(interface::Sampler sampler) {
    Renderer::getSingleton()->setTextureSampler(handle, sampler.handle);
  }
  friend class Material;
  friend class Renderer;
};

struct Material {
private:
  handle::Material handle;

public:
  Material() { handle = Renderer::getSingleton()->createMaterial(); }
  Material(const Material &other) {
    handle = other.handle;
    Renderer::getSingleton()->refMaterial(handle);
  }
  ~Material() { Renderer::getSingleton()->destroyMaterial(handle); }
  glm::vec4 getColorFactor() {
    return Renderer::getSingleton()->getMaterialColorFactor(handle);
  }
  glm::vec3 getEmissiveFactor() {
    return Renderer::getSingleton()->getMaterialEmissiveFactor(handle);
  }
  interface::Texture getNormalTexture() {
    handle::Texture texture =
        Renderer::getSingleton()->getMaterialNormalTexture(handle);
    Renderer::getSingleton()->refTexture(texture);
    return {texture};
  }
  interface::Texture getEmissiveTexture() {
    handle::Texture texture =
        Renderer::getSingleton()->getMaterialEmissiveTexture(handle);
    Renderer::getSingleton()->refTexture(texture);
    return {texture};
  }
  interface::Texture getOcclusionTexture() {
    handle::Texture texture =
        Renderer::getSingleton()->getMaterialOcclusionTexture(handle);
    Renderer::getSingleton()->refTexture(texture);
    return {texture};
  }
  interface::Texture getColorTexture() {
    handle::Texture texture =
        Renderer::getSingleton()->getMaterialColorTexture(handle);
    Renderer::getSingleton()->refTexture(texture);
    return {texture};
  }
  interface::Texture getMetallicRoughness() {
    handle::Texture texture =
        Renderer::getSingleton()->getMaterialMetallicRoughness(handle);
    Renderer::getSingleton()->refTexture(texture);
    return {texture};
  }
  float getNormalScale() {
    return Renderer::getSingleton()->getMaterialNormalScale(handle);
  }
  float getMetallicFactor() {
    return Renderer::getSingleton()->getMaterialMetallicFactor(handle);
  }
  float getRoughnessFactor() {
    return Renderer::getSingleton()->getMaterialRoughnessFactor(handle);
  }
  void setMaterialColorFactor(glm::vec4 color_factor) {
    Renderer::getSingleton()->setMaterialColorFactor(handle, color_factor);
  }
  void setMaterialEmissiveFactor(glm::vec3 emissive_factor) {
    Renderer::getSingleton()->setMaterialEmissiveFactor(handle,
                                                        emissive_factor);
  }
  void setMaterialNormalTexture(Texture texture) {
    Renderer::getSingleton()->setMaterialNormalTexture(handle, texture.handle);
  }
  void setMaterialEmissiveTexture(Texture texture) {
    Renderer::getSingleton()->setMaterialEmissiveTexture(handle,
                                                         texture.handle);
  }
  void setMaterialOcclusionTexture(Texture texture) {
    Renderer::getSingleton()->setMaterialOcclusionTexture(handle,
                                                          texture.handle);
  }
  void setMaterialColorTexture(Texture texture) {
    Renderer::getSingleton()->setMaterialColorTexture(handle, texture.handle);
  }
  void setMaterialMetallicRoughnessTexture(Texture texture) {
    Renderer::getSingleton()->setMaterialMetallicRoughness(handle,
                                                           texture.handle);
  }
  void setMaterialNormalScale(float normal_scale) {
    Renderer::getSingleton()->setMaterialNormalScale(handle, normal_scale);
  }
  void setMaterialMetallicFactor(float metallic_factor) {
    Renderer::getSingleton()->setMaterialMetallicFactor(handle,
                                                        metallic_factor);
  }
  void setMaterialRoughnessFactor(float roughness_factor) {
    Renderer::getSingleton()->setMaterialRoughnessFactor(handle,
                                                         roughness_factor);
  }
  friend class Renderer;
};

struct Transform {
  glm::mat4x4 rel_mtw_transform;
};

class Mesh {
private:
  handle::Mesh handle;

public:
  Mesh() { handle = Renderer::getSingleton()->createMesh(); }
  Mesh(std::shared_ptr<Vertex> vert_buffer,
       std::shared_ptr<uint16_t> index_buffer, uint32_t vert_count,
       uint32_t index_count) {
    handle = Renderer::getSingleton()->createMesh();
    Renderer::getSingleton()->uploadMeshBuffers(
        handle, vert_buffer, index_buffer, vert_count, index_count);
  }
  Mesh(const Mesh &other) {
    handle = other.handle;
    Renderer::getSingleton()->refMesh(handle);
  }
  ~Mesh() { Renderer::getSingleton()->destroyMesh(handle); }

  AABB getAABB() { return Renderer::getSingleton()->getAABB(handle); };
  void setAABB(AABB aabb) { Renderer::getSingleton()->setAABB(handle, aabb); };
  void uploadBuffers(std::shared_ptr<Vertex> vert_buffer,
                     std::shared_ptr<uint16_t> index_buffer,
                     uint32_t vert_count, uint32_t index_count) {
    Renderer::getSingleton()->uploadMeshBuffers(
        handle, vert_buffer, index_buffer, vert_count, index_count);
  }
  friend class Renderer;
};

class Shader {
private:
  handle::Shader handle;

public:
  Shader(const char *shader_file_path, uint32_t num_samplers = 0,
         uint32_t num_storage_textures = 0, uint32_t num_storage_buffers = 0,
         uint32_t num_uniform_buffers = 0,
         ShaderType shader_type = ShaderType::VERTEX) {
    handle = Renderer::getSingleton()->createShader(shader_file_path);
  }
  Shader(const Shader &other) {
    handle = other.handle;
    Renderer::getSingleton()->refShader(handle);
  }
  ~Shader() { Renderer::getSingleton()->destroyShader(handle); }

  uint32_t getNumSamplers(handle::Shader shader) {
    return Renderer::getSingleton()->getShaderNumSamplers(handle);
  }
  uint32_t getNumStorageTextures(handle::Shader shader) {
    return Renderer::getSingleton()->getShaderNumStorageTextures(handle);
  }
  uint32_t getNumStorageBuffers(handle::Shader shader) {
    return Renderer::getSingleton()->getShaderNumStorageBuffers(handle);
  }
  uint32_t getNumUniformBuffers(handle::Shader shader) {
    return Renderer::getSingleton()->getShaderNumUniformBuffers(handle);
  }
  ShaderType getType(handle::Shader shader) {
    return Renderer::getSingleton()->getShaderType(handle);
  }
};

struct DrawCommand {
public:
  const Mesh &mesh;
  const Material &material;
  glm::mat4x4 transform;

  DrawCommand(const Mesh &mesh, const Material &material, glm::mat4x4 transform)
      : mesh(mesh), material(material), transform(transform) {}

  DrawCommand(const DrawCommand &other)
      : mesh(other.mesh), material(other.material), transform(other.transform) {
  }
};

class Renderer {
public:
  static void
  draw(Texture target_texture,
       const std::variant<PerspectiveCamera, OrthogonalCamera> &camera,
       const glm::mat4x4 &camera_transform,
       const std::vector<DrawCommand> _draw_commands) {
    std::vector<::DrawCommand> draw_commands;
    for (auto &draw_command : _draw_commands) {
      draw_commands.push_back({.material = draw_command.material.handle,
                               .mesh = draw_command.mesh.handle,
                               .transform = draw_command.transform});
    }
    if (std::holds_alternative<PerspectiveCamera>(camera)) {
      ::Renderer::getSingleton()->drawToTexture(
          target_texture.handle, std::get<PerspectiveCamera>(camera).handle,
          camera_transform, draw_commands);
    } else {
      ::Renderer::getSingleton()->drawToTexture(
          target_texture.handle, std::get<OrthogonalCamera>(camera).handle,
          camera_transform, draw_commands);
    }
  }

  static uintptr_t getTexture(const Texture tex) {
    return ::Renderer::getSingleton()->getTexture(tex.handle);
  }
};

}; // namespace interface