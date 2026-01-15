#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_video.h"
#include "entt/entity/fwd.hpp"
#include "fastgltf/types.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui_internal.h"
#include "loaders/gltf-loader.hpp"
#include "renderer/renderer.hpp"
#include "renderer/wrappers.hpp"
#include "scene/components.hpp"
#include "scene/scene-manager.hpp"
#include "scene/scene.hpp"
#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <optional>
#include <queue>
#include <string>

//! thread safety needed here
void loadGLTFCallback(void *userdata, const char *const *filelist, int filter) {
	if (filelist == nullptr || filelist[0] == nullptr) {
		return;
	}
	std::string gltf_path = filelist[0];
	load(gltf_path);
}

void handleEditorCameraMovement(Transform &trs, RE::Camera::Shared camera, glm::vec2 content_region) {
	ImGuiIO &io = ImGui::GetIO();

	glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	// WASD for movement
	if (io.MouseWheel) {
		trs.translate += forward * -1.0f * io.MouseWheel; // invert
	}
	if (io.MouseDown[ImGuiMouseButton_Middle]) {
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			//! better solution needed
			float camera_speed = 5.0f;
			float delta_x = camera_speed * (io.MouseDelta.x / content_region.x) * RE::Camera::getAspectRatio(camera.handle);
			float delta_y = camera_speed * (io.MouseDelta.y / content_region.y) * -1; // invert
			trs.translate += delta_x * right + delta_y * up;
		}
		// Middle mouse button for rotation
		else {
			float delta_x = io.MouseDelta.x;
			float delta_y = io.MouseDelta.y * -1; // invert
			trs.rotate *= glm::angleAxis(glm::radians(delta_x), glm::vec3(0, 1, 0));
			auto pos_dir = glm::normalize(trs.translate);
			auto ground_norm = glm::normalize(glm::cross(pos_dir, glm::vec3(0, 1, 0)));
			trs.rotate *= glm::angleAxis(glm::radians(delta_y), ground_norm);
		}
	}
}

void drawToolBar(SDL_Window *window) {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 25));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y + 25));
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGuiWindowFlags window_flags =
			0 | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Master DockSpace", NULL, window_flags);
	ImGuiID dockMain = ImGui::GetID("MyDockspace");

	// Save off menu bar height for later.
	auto menuBarHeight = ImGui::GetCurrentWindow()->MenuBarHeight;

	ImGui::DockSpace(dockMain);
	ImGui::End();
	ImGui::PopStyleVar(3);

	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, 0));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 0));
	ImGui::SetNextWindowViewport(viewport->ID);

	window_flags = 0 | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::Begin("Toolbar", NULL, window_flags);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load scenes from gltf")) {
				SDL_DialogFileFilter file_filters = { .name = "GLTF File",
					.pattern = "gltf" };
				SDL_ShowOpenFileDialog(loadGLTFCallback, nullptr, window, &file_filters,
						1, nullptr, false);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::PopStyleVar();
	ImGui::End();
}

glm::vec2 drawRenderResult(const Scene &scene,
		const RE::Texture::Shared &render_target) {
	ImGui::Begin("render-result");
	ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
	auto content_region_avail = ImGui::GetContentRegionAvail();
	ImGui::Image(
			static_cast<ImTextureID>(RE::getTexture(render_target.handle)),
			content_region_avail);
	ImGui::PopStyleVar();
	ImGui::End();
	return glm::vec2(content_region_avail.x, content_region_avail.y);
}

void drawRenderOptions(RE::Options &options) {
	ImGuiIO &io = ImGui::GetIO();
	ImGui::Begin("Renderer Options");
	ImGui::ColorEdit4("Clear Color:",
			glm::value_ptr(options.clear_color));
	static float f = 0.0f;
	static int counter = 0;

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / io.Framerate, io.Framerate);
	ImGui::End();
}

void drawTexturePreview(const RE::Texture::Handle &texture,
		const std::string &texture_name) {
	auto tex_ref = RE::getTexture(texture);
	uint32_t preview_width = 50;
	uint32_t preview_height = 50;
	ImGui::Text((texture_name + ": %dx%d").c_str(), RE::Texture::getWidth(texture),
			RE::Texture::getHeight(texture));
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 uv_min = ImVec2(0.0f, 0.0f); // Top-left
	ImVec2 uv_max = ImVec2(1.0f, 1.0f); // Lower-right
	ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize,
			std::max(1.0f, ImGui::GetStyle().ImageBorderSize));
	ImGui::ImageWithBg(tex_ref, ImVec2(preview_width, preview_height), uv_min,
			uv_max, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	auto &io = ImGui::GetIO();
	if (ImGui::BeginItemTooltip()) {
		float region_sz = 32.0f;
		float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
		float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
		float zoom = 4.0f;
		if (region_x < 0.0f) {
			region_x = 0.0f;
		} else if (region_x > RE::Texture::getWidth(texture) - region_sz) {
			region_x = RE::Texture::getWidth(texture) - region_sz;
		}
		if (region_y < 0.0f) {
			region_y = 0.0f;
		} else if (region_y > RE::Texture::getHeight(texture) - region_sz) {
			region_y = RE::Texture::getHeight(texture) - region_sz;
		}
		ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
		ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz,
				region_y + region_sz);
		ImVec2 uv0 =
				ImVec2((region_x) / preview_width, (region_y) / preview_height);
		ImVec2 uv1 = ImVec2((region_x + region_sz) / preview_width,
				(region_y + region_sz) / preview_height);
		ImGui::ImageWithBg(tex_ref, ImVec2(region_sz * zoom, region_sz * zoom), uv0,
				uv1, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::EndTooltip();
	}
	ImGui::PopStyleVar();
}

void drawComponent(Transform &component) {
	bool edit = false;
	edit |= ImGui::InputFloat3("Translate", glm::value_ptr(component.translate));
	edit |= ImGui::InputFloat4("Rotate", glm::value_ptr(component.rotate));
	edit |= ImGui::InputFloat3("Scale", glm::value_ptr(component.scale));
}

void drawComponent(RE::Mesh::Handle &component) {
}

void drawComponent(RE::Material::Handle &component) {
	float metallic_factor = RE::Material::getMetallicFactor(component);
	float roughness_factor = RE::Material::getRoughnessFactor(component);
	float normal_scale = RE::Material::getNormalScale(component);
	glm::vec4 color_factor = RE::Material::getColorFactor(component);
	glm::vec3 emissive_factor = RE::Material::getEmissiveFactor(component);
	if (ImGui::InputFloat("Metallic Factor", &metallic_factor)) {
		RE::Material::setMetallicFactor(component, metallic_factor);
	}
	if (ImGui::InputFloat("Roughness Factor", &roughness_factor)) {
		RE::Material::setRoughnessFactor(component, roughness_factor);
	}
	if (ImGui::InputFloat("Normal Scale", &normal_scale)) {
		RE::Material::setMetallicFactor(component, normal_scale);
	}
	if (ImGui::InputFloat4("Color Factor", glm::value_ptr(color_factor))) {
		RE::Material::setColorFactor(component, color_factor);
	}
	if (ImGui::InputFloat3("Emissive Factor", glm::value_ptr(emissive_factor))) {
		RE::Material::setEmissiveFactor(component, emissive_factor);
	}
	if (RE::Texture::isValid(RE::Material::getColorTexture(component))) {
		drawTexturePreview(RE::Material::getColorTexture(component), "Color Texture");
	}
	if (RE::Texture::isValid(RE::Material::getEmissiveTexture(component))) {
		drawTexturePreview(RE::Material::getEmissiveTexture(component), "Emissive Texture");
	}
	if (RE::Texture::isValid(RE::Material::getNormalTexture(component))) {
		drawTexturePreview(RE::Material::getNormalTexture(component), "Emissive Texture");
	}
	if (RE::Texture::isValid(RE::Material::getOcclusionTexture(component))) {
		drawTexturePreview(RE::Material::getOcclusionTexture(component), "Emissive Texture");
	}
	if (RE::Texture::isValid(RE::Material::getMetallicRoughnessTexture(component))) {
		drawTexturePreview(RE::Material::getMetallicRoughnessTexture(component), "Emissive Texture");
	}
}

void drawComponent(RE::Camera::Handle &component) {
	if (RE::Camera::isOrthogonal(component)) {
		float xmag = RE::Camera::getXMag(component);
		float ymag = RE::Camera::getYMag(component);
		if (ImGui::InputFloat("XMag", &xmag)) {
			RE::Camera::setXMag(component, xmag);
		}
		if (ImGui::InputFloat("YMag", &ymag)) {
			RE::Camera::setYMag(component, ymag);
		}
	} else {
		float fov = RE::Camera::getFOV(component);
		float aspect_ratio = RE::Camera::getAspectRatio(component);
		if (ImGui::InputFloat("FOV", &fov)) {
			RE::Camera::setXMag(component, fov);
		}
		if (ImGui::InputFloat("Aspect Ratio", &aspect_ratio)) {
			RE::Camera::setYMag(component, aspect_ratio);
		}
	}
	float near_plane = RE::Camera::getNearPlane(component);
	float far_plane = RE::Camera::getFarPlane(component);
	if (ImGui::InputFloat("Near plane", &near_plane)) {
		RE::Camera::setXMag(component, near_plane);
	}
	if (ImGui::InputFloat("Far plane", &far_plane)) {
		RE::Camera::setYMag(component, far_plane);
	}
}

entt::entity active_node = entt::null;

void drawNodeRecursive(Scene &scene, const entt::entity node,
		ImGuiTreeNodeFlags base_flags) {
	std::optional<fastgltf::MaybeSmallVector<Child>> children;
	if (scene.nodes.any_of<fastgltf::MaybeSmallVector<Child>>(node)) {
		children = scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(node);
	}
	if (not children.has_value()) {
		base_flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (active_node == node) {
		base_flags |= ImGuiTreeNodeFlags_Selected;
	}
	std::string node_name;
	if (scene.nodes.any_of<Tag>(node)) {
		node_name = scene.nodes.get<Tag>(node).name +
				std::to_string(static_cast<uint32_t>(node));
	} else {
		node_name = "Untitled##" + std::to_string(static_cast<uint32_t>(node));
	}

	if (ImGui::TreeNodeEx(node_name.c_str(), base_flags)) {
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
				ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			active_node = node;
		}
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::SetDragDropPayload("Node", &node, sizeof(node));
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Node")) {
				IM_ASSERT(payload->DataSize == sizeof(entt::entity));
				entt::entity payload_node = *static_cast<entt::entity *>(payload->Data);
				if (scene.nodes.any_of<Parent>(payload_node)) {
					const auto &parent = scene.nodes.get<Parent>(payload_node);
					auto &children = const_cast<fastgltf::MaybeSmallVector<Child> &>(
							scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(parent.node));
					for (auto it = children.begin(); it != children.end(); it++) {
						if (it->node == payload_node) {
							children.erase(it);
							break;
						}
					}
				}
				scene.nodes.emplace_or_replace<Parent>(payload_node, node);
				if (not scene.nodes.any_of<fastgltf::MaybeSmallVector<Child>>(node)) {
					scene.nodes.emplace<fastgltf::MaybeSmallVector<Child>>(node);
				}
				scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(node).push_back(
						{ payload_node });
			}
			ImGui::EndDragDropTarget();
		}
		if (not scene.nodes.any_of<Disabled>(node) and children.has_value()) {
			for (const auto child : *children) {
				drawNodeRecursive(scene, child.node, base_flags);
			}
		}
		ImGui::TreePop();
	}
}

void drawSceneGraph(SceneManager *scene_manager) {
	if (0 > scene_manager->active_scene_index ||
			scene_manager->active_scene_index >= scene_manager->scenes.size()) {
		return;
	}
	ImGui::ShowDemoWindow();

	ImGui::Begin("Scene Graph");

	auto &scene = scene_manager->scenes[scene_manager->active_scene_index];
	std::string scene_name =
			"Untitled" + std::to_string(scene_manager->active_scene_index);
	if (not scene.name.empty()) {
		scene_name =
				scene.name + "##" + std::to_string(scene_manager->active_scene_index);
	}
	if (ImGui::BeginCombo("Scenes", scene_name.c_str())) {
		for (int n = 0; n < scene_manager->scenes.size(); n++) {
			std::string other_scene_name = "Untitled" + std::to_string(n);
			if (not scene_manager->scenes[n].name.empty()) {
				other_scene_name =
						scene_manager->scenes[n].name + +"##" + std::to_string(n);
			}
			const bool is_selected = (scene_manager->active_scene_index == n);
			if (ImGui::Selectable(other_scene_name.c_str(), is_selected)) {
				scene_manager->active_scene_index = n;
				active_node = entt::null;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGuiTreeNodeFlags base_flags =
			ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_DrawLinesToNodes;
	if (ImGui::TreeNodeEx("Hierarchy", base_flags)) {
		std::queue<entt::entity> q;
		auto root_node_view = scene.nodes.view<entt::entity>(); //! use exclude view
		for (auto root_node : root_node_view) {
			if (not scene.nodes.any_of<Parent>(root_node)) {
				drawNodeRecursive(scene, root_node, base_flags);
			}
		}
		ImGui::TreePop();
	}
	if (scene.nodes.valid(active_node)) {
		ImGui::Begin("Components");
		bool is_disabled = scene.nodes.any_of<Disabled>(active_node);
		if (ImGui::Checkbox("Disabled", &is_disabled)) {
			if (is_disabled) {
				scene.nodes.emplace<Disabled>(active_node);
			} else {
				scene.nodes.remove<Disabled>(active_node);
			}
		}
		if (scene.nodes.any_of<Transform>(active_node) and
				not ImGui::CollapsingHeader("Transform")) {
			drawComponent(scene.nodes.get<Transform>(active_node));
		}
		if (scene.nodes.any_of<RenderableMesh>(active_node) and
				not ImGui::CollapsingHeader("Mesh")) {
			drawComponent(scene.nodes.get<RenderableMesh>(active_node).mesh.handle);
		}
		if (scene.nodes.any_of<RE::Camera::Shared>(active_node) and
				not ImGui::CollapsingHeader("Camera")) {
			drawComponent(scene.nodes.get<RE::Camera::Shared>(active_node).handle);
		}
		ImGui::End();
	}
	ImGui::End();
}