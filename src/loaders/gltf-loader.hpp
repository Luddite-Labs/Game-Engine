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

[[nodiscard]] std::vector<interface::Sampler>
loadSamplers(const fastgltf::Asset &asset) {
	std::vector<interface::Sampler> samplers;
	for (const auto &asset_sampler : asset.samplers) {
		interface::Sampler sampler{};
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

[[nodiscard]] std::vector<interface::Texture>
loadTextures(const fastgltf::Asset &asset,
		const std::vector<interface::Sampler> &samplers,
		const std::vector<Image> &images) {
	std::vector<interface::Texture> textures;
	//! use the right format for texture 
	for (auto &asset_texture : asset.textures) {
		const auto &image = images[asset_texture.imageIndex.value()];
		interface::Texture texture{ image.width, image.height };
		texture.uploadBuffer(image.buffer, 0,
				static_cast<size_t>(image.width) *
						static_cast<size_t>(image.height) *
						static_cast<size_t>(image.channels));
		if (asset_texture.samplerIndex.has_value()) {
			const auto &sampler = samplers[asset_texture.samplerIndex.value()];
			texture.setSampler(sampler);
		}
		textures.push_back(texture);
	}
	return std::move(textures);
}

[[nodiscard]] std::vector<interface::Material>
loadMaterials(const fastgltf::Asset &asset,
		const std::vector<interface::Texture> &textures) {
	std::vector<interface::Material> materials;
	for (const auto &asset_material : asset.materials) {
		interface::Material material;
		material.setColorFactor(
				glm::make_vec4(asset_material.pbrData.baseColorFactor.data()));
		material.setEmissiveFactor(
				glm::make_vec4(asset_material.emissiveFactor.data()));
		material.setMetallicFactor(asset_material.pbrData.metallicFactor);
		material.setRoughnessFactor(asset_material.pbrData.roughnessFactor);
		if (asset_material.pbrData.baseColorTexture.has_value()) {
			material.setColorTexture(
					textures[asset_material.pbrData.baseColorTexture
									->textureIndex]); // tex coord index transform
		}
		if (asset_material.normalTexture.has_value()) {
			material.setNormalTexture(
					textures[asset_material.normalTexture
									->textureIndex]); // tex coord index transform
		}
		if (asset_material.emissiveTexture.has_value()) {
			material.setEmissiveTexture(
					textures[asset_material.emissiveTexture
									->textureIndex]); // tex coord index transform
		}
		if (asset_material.occlusionTexture.has_value()) {
			material.setOcclusionTexture(
					textures[asset_material.occlusionTexture
									->textureIndex]); // tex coord index transform
		}
		if (asset_material.pbrData.metallicRoughnessTexture.has_value()) {
			material.setMetallicRoughnessTexture(
					textures[asset_material.pbrData.metallicRoughnessTexture
									->textureIndex]); // tex coord index transform
		}
		materials.push_back(material);
	}
	return std::move(materials);
}

[[nodiscard]] std::vector<interface::Mesh>
loadMeshes(const fastgltf::Asset &asset, const std::vector<interface::Material> &materials) {
	std::vector<interface::Mesh> meshes;
	for (const auto &asset_mesh : asset.meshes) {
		interface::MeshData mesh_data;
		for (auto &&primitive : asset_mesh.primitives) {
			if (primitive.type != fastgltf::PrimitiveType::Triangles) {
				continue;
			}
			mesh_data.primitives.emplace_back();
			auto &prim_data = mesh_data.primitives.back();
			prim_data.type = data::PrimitiveType::TRIANGLELIST;
			if (primitive.materialIndex.has_value()) {
				prim_data.material = materials[primitive.materialIndex.value()];
			}
			// load indexes
			{
				const fastgltf::Accessor &indexaccessor =
						asset.accessors[primitive.indicesAccessor.value()];
				prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::INDEX)] =
						std::make_unique<uint8_t[]>(indexaccessor.count * sizeof(uint16_t));
				prim_data.index_count = indexaccessor.count;
				uint16_t arr_idx = 0;
				fastgltf::iterateAccessor<uint32_t>(
						asset, indexaccessor, [&](uint32_t idx) {
							uint16_t cast_idx = static_cast<uint16_t>(idx);
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::INDEX)].get() + (arr_idx * sizeof(uint16_t)), &cast_idx, sizeof(uint16_t));
							arr_idx++;
						});
			}

			// load vertex positions
			{
				const fastgltf::Accessor &posAccessor =
						asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
				prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::POSITION)] =
						std::make_unique<uint8_t[]>(posAccessor.count * sizeof(glm::vec3));
				prim_data.vert_count = posAccessor.count;
				fastgltf::iterateAccessorWithIndex<glm::vec3>(
						asset, posAccessor, [&](glm::vec3 v, size_t index) {
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::POSITION)].get() + (index * sizeof(glm::vec3)), glm::value_ptr(v), sizeof(glm::vec3));
						});
			}

			// load UV
			{
				auto at_it = primitive.findAttribute("TEXCOORD_0");
				if (at_it != primitive.attributes.end()) {
					const fastgltf::Accessor &uvAccessor =
							asset.accessors[at_it->accessorIndex];
					prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::UV)] =
							std::make_unique<uint8_t[]>(uvAccessor.count * sizeof(glm::vec2));
					fastgltf::iterateAccessorWithIndex<glm::vec2>(
							asset, uvAccessor, [&](glm::vec2 v, size_t index) {
								glm::vec2 pv = v;
								memcpy(prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::UV)].get() + (index * sizeof(glm::vec2)), glm::value_ptr(pv), sizeof(glm::vec2));
							});
				}
			}

			// load vertex normals
			auto normals = primitive.findAttribute("NORMAL");
			if (normals != primitive.attributes.end()) {
				const fastgltf::Accessor &normalAccessor =
						asset.accessors[normals->accessorIndex];
				prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::NORMAL)] =
						std::make_unique<uint8_t[]>(normalAccessor.count * sizeof(glm::vec3));
				fastgltf::iterateAccessorWithIndex<glm::vec3>(
						asset, asset.accessors[(*normals).accessorIndex],
						[&](glm::vec3 v, size_t index) {
							memcpy(prim_data.attrs_data[static_cast<uint32_t>(data::VertAttributeIndex::NORMAL)].get() + (index * sizeof(glm::vec3)), glm::value_ptr(v), sizeof(glm::vec3));
						});
			}
		}
		meshes.emplace_back(mesh_data);
	}
	return std::move(meshes);
}

[[nodiscard]] std::vector<interface::Camera>
loadCameras(const fastgltf::Asset &asset) {
	std::vector<interface::Camera> cameras;
	for (auto &camera : asset.cameras) {
		if (std::holds_alternative<fastgltf::Camera::Perspective>(camera.camera)) {
			auto &camera_data =
					std::get<fastgltf::Camera::Perspective>(camera.camera);
			interface::Camera persp_camera;
			if (camera_data.aspectRatio.has_value()) {
				persp_camera.setAspectRatio(camera_data.aspectRatio.value());
			}
			persp_camera.setFOV(camera_data.yfov);
			persp_camera.setNearPlane(camera_data.znear);
			persp_camera.setFarPlane(100.0f);
			if (camera_data.zfar.has_value()) {
				persp_camera.setFarPlane(camera_data.zfar.value());
			}
			cameras.push_back(persp_camera);
		} else {
			auto &camera_data =
					std::get<fastgltf::Camera::Orthographic>(camera.camera);
			interface::Camera ortho_camera;
			ortho_camera.setXMag(camera_data.xmag);
			ortho_camera.setYMag(camera_data.ymag);
			ortho_camera.setNearPlane(camera_data.znear);
			ortho_camera.setFarPlane(camera_data.zfar);
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

	std::vector<interface::Sampler> samplers = loadSamplers(asset.get());
	std::vector<Image> images = loadImages(asset.get(), gltf_path);
	std::vector<interface::Texture> textures =
			loadTextures(asset.get(), samplers, images);
	std::vector<interface::Material> materials =
			loadMaterials(asset.get(), textures);
	std::vector<interface::Mesh> meshes = loadMeshes(asset.get(), materials);
	std::vector<interface::Camera> cameras = loadCameras(asset.get());

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
				scene.nodes.emplace<interface::Camera>(
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
		if (scene.nodes.view<interface::Camera>().size() == 0) {
			auto node = scene.nodes.create();
			interface::Camera camera;
			camera.setDefaultPerspective();
			scene.nodes.emplace<interface::Camera>(node, camera);
			scene.nodes.emplace<Tag>(node, "Default Camera");
			scene.active_camera_node = node;
		}
	}

	scene_manager->active_scene_index = 0;
	if (asset->defaultScene.has_value()) {
		scene_manager->active_scene_index = asset->defaultScene.value();
	}
}