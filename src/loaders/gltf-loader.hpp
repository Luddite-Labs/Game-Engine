#pragma once

#include "fastgltf/math.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "misc/log.hpp"
#include "renderer/interface.hpp"
#include <cstdint>
#include <variant>
#define STB_IMAGE_IMPLEMENTATION
#include "glm/gtc/type_ptr.hpp"
#include "renderer/renderer.hpp"
#include "scene/scene-manager.hpp"
#include "scene/scene.hpp"
#include <string>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <renderer/common.hpp>
#include <scene/scene.hpp>
#include <stb_image.h>

std::tuple<interface::Mesh, interface::Material>
loadMesh(const fastgltf::Asset &asset, const fastgltf::Mesh &asset_mesh,
         const std::vector<interface::Texture> &textures) {
  interface::Mesh mesh{};
  interface::Material mat{};
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  indices.clear();
  vertices.clear();
  for (auto &&primitive : asset_mesh.primitives) {
    size_t initial_vtx = vertices.size();
    // load indexes
    {
      const fastgltf::Accessor &indexaccessor =
          asset.accessors[primitive.indicesAccessor.value()];
      indices.reserve(indices.size() + indexaccessor.count);

      fastgltf::iterateAccessor<uint32_t>(
          asset, indexaccessor, [&](uint32_t idx) {
            indices.push_back(static_cast<uint16_t>(idx + initial_vtx));
          });
    }

    // load vertex positions
    {
      const fastgltf::Accessor &posAccessor =
          asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
      vertices.resize(vertices.size() + posAccessor.count);

      fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset, posAccessor, [&](glm::vec3 v, size_t index) {
            Vertex newvtx;
            newvtx.position = v;
            newvtx.normal = {1, 0, 0};
            vertices[initial_vtx + index] = newvtx;
          });
    }

    // load UV
    {
      auto at_it = primitive.findAttribute("TEXCOORD_0");
      if (at_it != primitive.attributes.end()) {
        const fastgltf::Accessor &uvAccessor =
            asset.accessors[at_it->accessorIndex];
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset, uvAccessor, [&](glm::vec2 v, size_t index) {
              glm::vec2 pv = v;
              vertices[initial_vtx + index].uv = pv;
            });
      }
    }

    // load vertex normals
    auto normals = primitive.findAttribute("NORMAL");
    if (normals != primitive.attributes.end()) {

      fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset, asset.accessors[(*normals).accessorIndex],
          [&](glm::vec3 v, size_t index) {
            vertices[initial_vtx + index].normal = v;
          });
    }
    if (primitive.materialIndex.has_value()) {
      auto &asset_material = asset.materials[primitive.materialIndex.value()];
      mat.setMaterialColorFactor(
          glm::make_vec4(asset_material.pbrData.baseColorFactor.data()));
      mat.setMaterialEmissiveFactor(
          glm::make_vec4(asset_material.emissiveFactor.data()));
      mat.setMaterialMetallicFactor(asset_material.pbrData.metallicFactor);
      mat.setMaterialRoughnessFactor(asset_material.pbrData.roughnessFactor);
      if (asset_material.pbrData.baseColorTexture.has_value()) {
        mat.setMaterialColorTexture(
            textures[asset_material.pbrData.baseColorTexture
                         ->textureIndex]); // tex coord index transform
      }
      if (asset_material.normalTexture.has_value()) {
        mat.setMaterialNormalTexture(
            textures[asset_material.normalTexture
                         ->textureIndex]); // tex coord index transform
      }
      if (asset_material.emissiveTexture.has_value()) {
        mat.setMaterialEmissiveTexture(
            textures[asset_material.emissiveTexture
                         ->textureIndex]); // tex coord index transform
      }
      if (asset_material.occlusionTexture.has_value()) {
        mat.setMaterialOcclusionTexture(
            textures[asset_material.occlusionTexture
                         ->textureIndex]); // tex coord index transform
      }
      if (asset_material.pbrData.metallicRoughnessTexture.has_value()) {
        mat.setMaterialMetallicRoughnessTexture(
            textures[asset_material.pbrData.metallicRoughnessTexture
                         ->textureIndex]); // tex coord index transform
      }
    }
  }
  //! redesign interface
  mesh.uploadBuffers(
      std::shared_ptr<Vertex>(vertices.data(), [](Vertex *) {}),
      std::shared_ptr<uint16_t>(indices.data(), [](uint16_t *) {}),
      static_cast<uint32_t>(vertices.size()),
      static_cast<uint32_t>(indices.size()));
  return {mesh, mat};
}

void load(std::string file_path) {
  auto scene_manager = SceneManager::getSingleton();
  scene_manager->scenes.emplace_back();
  Scene &scene = scene_manager->scenes.back();

  fastgltf::Extensions extensions;
  fastgltf::Parser parser(extensions);
  auto gltfFile = fastgltf::GltfDataBuffer::FromPath(file_path + "/scene.gltf");
  auto asset = parser.loadGltf(gltfFile.get(), file_path,
                               fastgltf::Options::GenerateMeshIndices |
                                   fastgltf::Options::LoadExternalBuffers);

  std::vector<interface::Texture> textures;
  for (auto &tex : asset->textures) {
    auto image = asset->images[tex.imageIndex.value()];
    interface::Sampler sampler{};
    if (tex.samplerIndex.has_value()) {
      auto asset_sampler = asset->samplers[tex.samplerIndex.value()];
      if (asset_sampler.magFilter.has_value()) {
        switch (asset_sampler.magFilter.value()) {
        case fastgltf::Filter::Nearest:
          sampler.setMagFilter(SamplerFilteringModes::NEAREST);
          break;
        case fastgltf::Filter::Linear:
          sampler.setMagFilter(SamplerFilteringModes::LINEAR);
          break;
        default: //! implement all filtering modes
          sampler.setMagFilter(SamplerFilteringModes::NEAREST);
        }
      }
      if (asset_sampler.minFilter.has_value()) {
        switch (asset_sampler.minFilter.value()) {
        case fastgltf::Filter::Nearest:
          sampler.setMinFilter(SamplerFilteringModes::NEAREST);
          break;
        case fastgltf::Filter::Linear:
          sampler.setMinFilter(SamplerFilteringModes::LINEAR);
          break;
        default: //! implement all filtering modes
          sampler.setMinFilter(SamplerFilteringModes::NEAREST);
        }
      }
      switch (asset_sampler.wrapT) {
      case fastgltf::Wrap::Repeat:
        sampler.setUAddressing(SamplerAddressingModes::REPEAT);
        break;
      case fastgltf::Wrap::ClampToEdge:
        sampler.setUAddressing(SamplerAddressingModes::CLAMP_TO_EDGE);
        break;
      case fastgltf::Wrap::MirroredRepeat:
        sampler.setUAddressing(SamplerAddressingModes::MIRRORED_REPEAT);
        break;
      }
      switch (asset_sampler.wrapS) {
      case fastgltf::Wrap::Repeat:
        sampler.setVAddressing(SamplerAddressingModes::REPEAT);
        break;
      case fastgltf::Wrap::ClampToEdge:
        sampler.setVAddressing(SamplerAddressingModes::CLAMP_TO_EDGE);
        break;
      case fastgltf::Wrap::MirroredRepeat:
        sampler.setVAddressing(SamplerAddressingModes::MIRRORED_REPEAT);
        break;
      }
    }
    std::visit(
        fastgltf::visitor{
            [](auto &arg) {},
            [&](fastgltf::sources::URI &filePath) {
              assert(filePath.fileByteOffset ==
                     0); // We don't support offsets with stbi.
              assert(filePath.uri.isLocalPath()); // We're only capable of
                                                  // loading local files.
              int width, height, nrChannels;
              const std::string path =
                  file_path + std::string(filePath.uri.path());
              unsigned char *data =
                  stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
              interface::Texture tex(width, height);
              tex.uploadBuffer(
                  std::shared_ptr<uint8_t>(
                      data, [](uint8_t *data) { stbi_image_free(data); }),
                  0, width * height * 4 * sizeof(uint8_t));
              tex.setSampler(sampler);
              textures.push_back(tex);
            },
            [&](fastgltf::sources::Array &vector) {
              int width, height, nrChannels;
              unsigned char *data = stbi_load_from_memory(
                  reinterpret_cast<const stbi_uc *>(vector.bytes.data()),
                  static_cast<int>(vector.bytes.size()), &width, &height,
                  &nrChannels, 4);
              interface::Texture tex(width, height);
              tex.uploadBuffer(
                  std::shared_ptr<uint8_t>(
                      data, [](uint8_t *data) { stbi_image_free(data); }),
                  0, width * height * 4 * sizeof(uint8_t));
              tex.setSampler(sampler);
              textures.push_back(tex);
              stbi_image_free(data);
            },
            [&](fastgltf::sources::BufferView &view) {
              // skip
            },
        },
        image.data);
  }

  for (auto &asset_node : asset->nodes) {
    if (not asset_node.meshIndex.has_value()) {
      continue;
    }
    Node node{};
    auto mesh_data = loadMesh(
        asset.get(), asset->meshes[asset_node.meshIndex.value()], textures);
    node.mesh = std::get<0>(mesh_data);
    node.material = std::get<1>(mesh_data);
    if (std::holds_alternative<fastgltf::math::fmat<4, 4>>(
            asset_node.transform)) {
      node.transform = glm::make_mat4x4(
          std::get<fastgltf::math::fmat<4, 4>>(asset_node.transform).data());
    } else {
      auto &trs = std::get<fastgltf::TRS>(asset_node.transform);
      auto t = glm::make_vec3(trs.translation.data());
      auto r = glm::make_quat(trs.rotation.data());
      auto s = glm::make_vec3(trs.scale.data());
      auto tmat = glm::translate(glm::mat4x4(1), t);
      auto rmat = glm::mat4_cast(r);
      auto smat = glm::scale(glm::mat4x4(1), s);
      node.transform = tmat * rmat * smat;
    }
    scene.nodes.push_back(node);
  }

  for (auto &camera : asset->cameras) {
    if (std::holds_alternative<fastgltf::Camera::Perspective>(camera.camera)) {
      auto &camera_data =
          std::get<fastgltf::Camera::Perspective>(camera.camera);
      interface::PerspectiveCamera persp_camera(1.77);
      if (camera_data.aspectRatio.has_value()) {
        persp_camera.setCameraAspectRatio(camera_data.aspectRatio.value());
      }
      persp_camera.setCameraFOV(camera_data.yfov);
      persp_camera.setCameraNearPlane(camera_data.znear);
      persp_camera.setCameraFarPlane(100.0f);
      if (camera_data.zfar.has_value()) {
        persp_camera.setCameraFarPlane(camera_data.zfar.value());
      }
      scene.cameras.push_back(persp_camera);
    } else {
      auto &camera_data =
          std::get<fastgltf::Camera::Orthographic>(camera.camera);
      interface::OrthogonalCamera ortho_camera;
      ortho_camera.setCameraXMag(camera_data.xmag);
      ortho_camera.setCameraYMag(camera_data.ymag);
      ortho_camera.setCameraNearPlane(camera_data.znear);
      ortho_camera.setCameraFarPlane(camera_data.zfar);
      scene.cameras.push_back(ortho_camera);
    }
  }
  if (scene.cameras.size() == 0) {
    scene.cameras.push_back(interface::PerspectiveCamera(1.77));
  }
  scene.active_camera_index = 0;
}