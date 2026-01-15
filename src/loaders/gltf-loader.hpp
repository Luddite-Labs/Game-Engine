#pragma once

#include "entt/entity/fwd.hpp"
#include "fastgltf/math.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "misc/log.hpp"
#include "renderer/wrappers.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <queue>
#include <variant>
#include <vector>
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
#include <glm/gtx/matrix_decompose.hpp>
#include <renderer/types.hpp>
#include <scene/components.hpp>
#include <scene/scene.hpp>
#include <stb_image.h>

struct Image {
	std::shared_ptr<uint8_t> buffer;
	uint32_t width;
	uint32_t height;
	uint8_t channels;
};

[[nodiscard]] std::vector<RE::Sampler::Shared>
loadSamplers(const fastgltf::Asset &asset) {
	std::vector<RE::Sampler::Shared> samplers;
	for (const auto &asset_sampler : asset.samplers) {
		RE::Sampler::Shared sampler = RE::Sampler::create();
		if (asset_sampler.magFilter.has_value()) {
			switch (asset_sampler.magFilter.value()) {
				case fastgltf::Filter::Nearest:
					RE::Sampler::setMagFilter(sampler.handle, RE::Sampler::FilteringModes::NEAREST);
					break;
				case fastgltf::Filter::Linear:
					RE::Sampler::setMagFilter(sampler.handle, RE::Sampler::FilteringModes::LINEAR);
					break;
				default: //! implement all filtering modes
					RE::Sampler::setMagFilter(sampler.handle, RE::Sampler::FilteringModes::NEAREST);
			}
		}
		if (asset_sampler.minFilter.has_value()) {
			switch (asset_sampler.minFilter.value()) {
				case fastgltf::Filter::Nearest:
					RE::Sampler::setMinFilter(sampler.handle, RE::Sampler::FilteringModes::NEAREST);
					break;
				case fastgltf::Filter::Linear:
					RE::Sampler::setMinFilter(sampler.handle, RE::Sampler::FilteringModes::LINEAR);
					break;
				default: //! implement all filtering modes
					RE::Sampler::setMinFilter(sampler.handle, RE::Sampler::FilteringModes::NEAREST);
			}
		}
		switch (asset_sampler.wrapT) {
			case fastgltf::Wrap::Repeat:
				RE::Sampler::setUAddressing(sampler.handle, RE::Sampler::AddressingModes::REPEAT);
				break;
			case fastgltf::Wrap::ClampToEdge:
				RE::Sampler::setUAddressing(sampler.handle, RE::Sampler::AddressingModes::CLAMP_TO_EDGE);
				break;
			case fastgltf::Wrap::MirroredRepeat:
				RE::Sampler::setUAddressing(sampler.handle, RE::Sampler::AddressingModes::MIRRORED_REPEAT);
				break;
		}
		switch (asset_sampler.wrapS) {
			case fastgltf::Wrap::Repeat:
				RE::Sampler::setVAddressing(sampler.handle, RE::Sampler::AddressingModes::REPEAT);
				break;
			case fastgltf::Wrap::ClampToEdge:
				RE::Sampler::setVAddressing(sampler.handle, RE::Sampler::AddressingModes::CLAMP_TO_EDGE);
				break;
			case fastgltf::Wrap::MirroredRepeat:
				RE::Sampler::setVAddressing(sampler.handle, RE::Sampler::AddressingModes::MIRRORED_REPEAT);
				break;
		}
		samplers.push_back(sampler);
	}
	return std::move(samplers);
}

[[nodiscard]] std::vector<Image> loadImages(const fastgltf::Asset &asset,
		const std::string &gltf_path) {
	std::vector<Image> images;
	for (auto image : asset.images) {
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
									(gltf_path + "/" + std::string(filePath.uri.path()));
							uint8_t *data =
									stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
							images.push_back(
									{ .buffer = std::shared_ptr<uint8_t>(
											  data, [](uint8_t *data) { stbi_image_free(data); }),
											.width = static_cast<uint32_t>(width),
											.height = static_cast<uint32_t>(height),
											.channels = 4 });
						},
						[&](fastgltf::sources::Array &vector) {
							int width, height, nrChannels;
							unsigned char *data = stbi_load_from_memory(
									reinterpret_cast<const stbi_uc *>(vector.bytes.data()),
									static_cast<int>(vector.bytes.size()), &width, &height,
									&nrChannels, 4);
							images.push_back(
									{ .buffer = std::shared_ptr<uint8_t>(
											  data, [](uint8_t *data) { stbi_image_free(data); }),
											.width = static_cast<uint32_t>(width),
											.height = static_cast<uint32_t>(height),
											.channels = 4 });
						},
						[&](fastgltf::sources::BufferView &view) {
							auto &bufferView = asset.bufferViews[view.bufferViewIndex];
							auto &buffer = asset.buffers[bufferView.bufferIndex];
							// Yes, we've already loaded every buffer into some GL buffer.
							// However, with GL it's simpler to just copy the buffer data
							// again for the texture. Besides, this is just an example.
							std::visit(
									fastgltf::visitor{
											// We only care about VectorWithMime here, because we
											// specify
											// LoadExternalBuffers, meaning
											// all buffers are already loaded into a vector.
											[](auto &arg) {},
											[&](fastgltf::sources::Array &vector) {
												int width, height, nrChannels;
												unsigned char *data = stbi_load_from_memory(
														reinterpret_cast<const stbi_uc *>(
																vector.bytes.data() + bufferView.byteOffset),
														static_cast<int>(bufferView.byteLength), &width,
														&height, &nrChannels, 4);
												images.push_back(
														{ .buffer = std::shared_ptr<uint8_t>(
																  data,
																  [](uint8_t *data) { stbi_image_free(data); }),
																.width = static_cast<uint32_t>(width),
																.height = static_cast<uint32_t>(height),
																.channels = 4 });
											} },
									buffer.data);
						},
				},
				image.data);
	}
	return std::move(images);
}

[[nodiscard]] std::vector<RE::Texture::Shared>
loadTextures(const fastgltf::Asset &asset,
		const std::vector<RE::Sampler::Shared> &samplers,
		const std::vector<Image> &images) {
	std::vector<RE::Texture::Shared> textures;
	//! use the right format for texture
	for (auto &asset_texture : asset.textures) {
		const auto &image = images[asset_texture.imageIndex.value()];
		RE::Texture::Shared texture = RE::Texture::create(image.width, image.height);
		RE::Texture::uploadBuffer(
				texture.handle, image.buffer, 0,
				static_cast<size_t>(image.width) *
						static_cast<size_t>(image.height) *
						static_cast<size_t>(image.channels));
		if (asset_texture.samplerIndex.has_value()) {
			const auto &sampler = samplers[asset_texture.samplerIndex.value()];
			RE::Texture::setSampler(texture.handle, sampler.handle);
		}
		textures.push_back(texture);
	}
	return std::move(textures);
}

[[nodiscard]] std::vector<RE::Material::Shared>
loadMaterials(const fastgltf::Asset &asset,
		const std::vector<RE::Texture::Shared> &textures) {
	std::vector<RE::Material::Shared> materials;
	for (const auto &asset_material : asset.materials) {
		RE::Material::Shared material = RE::Material::create();
		RE::Material::setColorFactor(material.handle,
				glm::make_vec4(asset_material.pbrData.baseColorFactor.data()));
		RE::Material::setEmissiveFactor(material.handle,
				glm::make_vec4(asset_material.emissiveFactor.data()));
		RE::Material::setMetallicFactor(material.handle, asset_material.pbrData.metallicFactor);
		RE::Material::setRoughnessFactor(material.handle, asset_material.pbrData.roughnessFactor);
		if (asset_material.pbrData.baseColorTexture.has_value()) {
			RE::Material::setColorTexture(material.handle,
					textures[asset_material.pbrData.baseColorTexture
									 ->textureIndex]
							.handle); // tex coord index transform
		}
		if (asset_material.normalTexture.has_value()) {
			RE::Material::setNormalTexture(material.handle,
					textures[asset_material.normalTexture
									 ->textureIndex]
							.handle); // tex coord index transform
		}
		if (asset_material.emissiveTexture.has_value()) {
			RE::Material::setEmissiveTexture(material.handle,
					textures[asset_material.emissiveTexture
									 ->textureIndex]
							.handle); // tex coord index transform
		}
		if (asset_material.occlusionTexture.has_value()) {
			RE::Material::setOcclusionTexture(material.handle,
					textures[asset_material.occlusionTexture
									 ->textureIndex]
							.handle); // tex coord index transform
		}
		if (asset_material.pbrData.metallicRoughnessTexture.has_value()) {
			RE::Material::setMetallicRoughnessTexture(material.handle,
					textures[asset_material.pbrData.metallicRoughnessTexture
									 ->textureIndex]
							.handle); // tex coord index transform
		}
		materials.push_back(material);
	}
	return std::move(materials);
}

[[nodiscard]] std::vector<RE::Mesh::Shared>
loadMeshes(const fastgltf::Asset &asset, const std::vector<RE::Material::Shared> &materials) {
	std::vector<RE::Mesh::Shared> meshes;
	for (const auto &asset_mesh : asset.meshes) {
		RE::Mesh::Arg mesh_data;
		for (auto &&primitive : asset_mesh.primitives) {
			if (primitive.type != fastgltf::PrimitiveType::Triangles) {
				continue;
			}
			mesh_data.primitives.emplace_back();
			auto &prim_data = mesh_data.primitives.back();
			prim_data.type = RE::Mesh::Primitive::Type::TRIANGLELIST;
			if (primitive.materialIndex.has_value()) {
				prim_data.material = materials[primitive.materialIndex.value()].handle;
			}
			// load indexes
			{
				const fastgltf::Accessor &indexaccessor =
						asset.accessors[primitive.indicesAccessor.value()];
				prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::INDEX)] =
						std::make_unique<uint8_t[]>(indexaccessor.count * sizeof(uint16_t));
				prim_data.index_count = indexaccessor.count;
				uint16_t arr_idx = 0;
				fastgltf::iterateAccessor<uint32_t>(
						asset, indexaccessor, [&](uint32_t idx) {
							uint16_t cast_idx = static_cast<uint16_t>(idx);
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::INDEX)].get() + (arr_idx * sizeof(uint16_t)), &cast_idx, sizeof(uint16_t));
							arr_idx++;
						});
			}

			// load vertex positions
			{
				const fastgltf::Accessor &posAccessor =
						asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::POSITION)] =
						std::make_unique<uint8_t[]>(posAccessor.count * sizeof(glm::vec3));
				prim_data.vert_count = posAccessor.count;
				fastgltf::iterateAccessorWithIndex<glm::vec3>(
						asset, posAccessor, [&](glm::vec3 v, size_t index) { //! maybe add epsilon to box size 
							mesh_data.aabb.min.x = std::min(v.x, mesh_data.aabb.min.x);
							mesh_data.aabb.min.y = std::min(v.y, mesh_data.aabb.min.y);
							mesh_data.aabb.min.z = std::min(v.z, mesh_data.aabb.min.z);
							mesh_data.aabb.max.x = std::max(v.x, mesh_data.aabb.max.x);
							mesh_data.aabb.max.y = std::max(v.y, mesh_data.aabb.max.y);
							mesh_data.aabb.max.z = std::max(v.z, mesh_data.aabb.max.z);
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::POSITION)].get() + (index * sizeof(glm::vec3)), glm::value_ptr(v), sizeof(glm::vec3));
						});
			}

			// load UV
			{
				auto at_it = primitive.findAttribute("TEXCOORD_0");
				if (at_it != primitive.attributes.end()) {
					const fastgltf::Accessor &uvAccessor =
							asset.accessors[at_it->accessorIndex];
					prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::UV)] =
							std::make_unique<uint8_t[]>(uvAccessor.count * sizeof(glm::vec2));
					fastgltf::iterateAccessorWithIndex<glm::vec2>(
							asset, uvAccessor, [&](glm::vec2 v, size_t index) {
								glm::vec2 pv = v;
								memcpy(prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::UV)].get() + (index * sizeof(glm::vec2)), glm::value_ptr(pv), sizeof(glm::vec2));
							});
				}
			}

			// load vertex normals
			auto normals = primitive.findAttribute("NORMAL");
			if (normals != primitive.attributes.end()) {
				const fastgltf::Accessor &normalAccessor =
						asset.accessors[normals->accessorIndex];
				prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::NORMAL)] =
						std::make_unique<uint8_t[]>(normalAccessor.count * sizeof(glm::vec3));
				fastgltf::iterateAccessorWithIndex<glm::vec3>(
						asset, asset.accessors[(*normals).accessorIndex],
						[&](glm::vec3 v, size_t index) {
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(RE::Vertex::AttributeIndex::NORMAL)].get() + (index * sizeof(glm::vec3)), glm::value_ptr(v), sizeof(glm::vec3));
						});
			}
		}
		RE::Mesh::Shared mesh = RE::Mesh::create(mesh_data);
		meshes.emplace_back(mesh);
	}
	return std::move(meshes);
}

[[nodiscard]] std::vector<RE::Camera::Shared>
loadCameras(const fastgltf::Asset &asset) {
	std::vector<RE::Camera::Shared> cameras;
	for (auto &camera : asset.cameras) {
		if (std::holds_alternative<fastgltf::Camera::Perspective>(camera.camera)) {
			auto &camera_data =
					std::get<fastgltf::Camera::Perspective>(camera.camera);
			RE::Camera::Shared persp_camera = RE::Camera::create();
			if (camera_data.aspectRatio.has_value()) {
				RE::Camera::setAspectRatio(persp_camera.handle, camera_data.aspectRatio.value());
			}
			RE::Camera::setFOV(persp_camera.handle, camera_data.yfov);
			RE::Camera::setNearPlane(persp_camera.handle, camera_data.znear);
			RE::Camera::setFarPlane(persp_camera.handle, 100.0f);
			if (camera_data.zfar.has_value()) {
				RE::Camera::setFarPlane(persp_camera.handle, camera_data.zfar.value());
			}
			cameras.push_back(persp_camera);
		} else {
			auto &camera_data =
					std::get<fastgltf::Camera::Orthographic>(camera.camera);
			RE::Camera::Shared ortho_camera = RE::Camera::create();
			RE::Camera::setXMag(ortho_camera.handle, camera_data.xmag);
			RE::Camera::setYMag(ortho_camera.handle, camera_data.ymag);
			RE::Camera::setNearPlane(ortho_camera.handle, camera_data.znear);
			RE::Camera::setFarPlane(ortho_camera.handle, camera_data.zfar);
			cameras.push_back(ortho_camera);
		}
		return std::move(cameras);
	}
	return std::move(cameras);
}

void load(std::string file_path) {
	std::string gltf_path = std::filesystem::path(file_path).parent_path();
	auto scene_manager = SceneManager::getSingleton();

	fastgltf::Extensions extensions;
	fastgltf::Parser parser(extensions);
	auto gltfFile = fastgltf::GltfDataBuffer::FromPath(file_path);
	auto asset = parser.loadGltf(gltfFile.get(), gltf_path,
			fastgltf::Options::GenerateMeshIndices |
					fastgltf::Options::LoadExternalBuffers |
					fastgltf::Options::DecomposeNodeMatrices);

	std::vector<RE::Sampler::Shared> samplers = loadSamplers(asset.get());
	std::vector<Image> images = loadImages(asset.get(), gltf_path);
	std::vector<RE::Texture::Shared> textures =
			loadTextures(asset.get(), samplers, images);
	std::vector<RE::Material::Shared> materials =
			loadMaterials(asset.get(), textures);
	std::vector<RE::Mesh::Shared> meshes = loadMeshes(asset.get(), materials);
	std::vector<RE::Camera::Shared> cameras = loadCameras(asset.get());

	for (const auto &asset_scene : asset->scenes) {
		scene_manager->scenes.emplace_back();
		Scene &scene = scene_manager->scenes.back();

		std::queue<std::pair<size_t, entt::entity>> q;
		for (const auto &root_node : asset_scene.nodeIndices) {
			q.push({ root_node, entt::null });
		}
		while (not q.empty()) {
			const auto [curr_node_index, parent_node] = q.front();
			q.pop();
			auto node = scene.nodes.create();
			const auto &asset_node = asset->nodes[curr_node_index];
			if (not asset_node.name.empty()) {
				scene.nodes.emplace<Tag>(node, std::string(asset_node.name));
			}
			if (parent_node != entt::null) {
				if (not scene.nodes.all_of<fastgltf::MaybeSmallVector<Child>>(
							parent_node)) {
					scene.nodes.emplace<fastgltf::MaybeSmallVector<Child>>(parent_node);
				} //! add tied update logic
				scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(parent_node)
						.push_back({ node });
				scene.nodes.emplace<Parent>(node,
						parent_node); //! add tied update logic
			}
			if (asset->nodes[curr_node_index].meshIndex.has_value()) {
				scene.nodes.emplace<RenderableMesh>(
						node, meshes[asset_node.meshIndex.value()]);
			}
			if (asset_node.cameraIndex.has_value()) {
				scene.nodes.emplace<RE::Camera::Shared>(
						node, cameras[asset_node.cameraIndex.value()]);
			}
			if (std::holds_alternative<fastgltf::math::fmat<4, 4>>(
						asset_node.transform)) {
				glm::vec3 s;
				glm::quat r;
				glm::vec3 t;
				glm::vec3 sk;
				glm::vec4 p;
				glm::decompose(glm::make_mat4x4(std::get<fastgltf::math::fmat<4, 4>>(
									   asset_node.transform)
											   .data()),
						s, r, t, sk, p);
				scene.nodes.emplace<Transform>(node, t, r, s);
			} else {
				auto &trs = std::get<fastgltf::TRS>(asset_node.transform);
				auto t = glm::make_vec3(trs.translation.data());
				auto r = glm::make_quat(trs.rotation.data());
				auto s = glm::make_vec3(trs.scale.data());
				scene.nodes.emplace<Transform>(node, t, r, s);
			}
			for (const auto &child_node_index : asset_node.children) {
				q.push({ child_node_index, node });
			}
		}
		if (not asset_scene.name.empty()) {
			scene.name = asset_scene.name;
		}
		if (scene.nodes.view<RE::Camera::Shared>().size() == 0) {
			auto node = scene.nodes.create();
			RE::Camera::Shared camera = RE::Camera::create();
			RE::Camera::setAspectRatio(camera.handle, 1.77);
			RE::Camera::setFOV(camera.handle, glm::radians(75.0f));
			RE::Camera::setNearPlane(camera.handle, 1.0f);
			RE::Camera::setFarPlane(camera.handle, 100.0f);
			scene.nodes.emplace<RE::Camera::Shared>(node, camera);
			scene.nodes.emplace<Tag>(node, "Default Camera");
			scene.active_camera_node = node;
		}
	}

	scene_manager->active_scene_index = 0;
	if (asset->defaultScene.has_value()) {
		scene_manager->active_scene_index = asset->defaultScene.value();
	}
}