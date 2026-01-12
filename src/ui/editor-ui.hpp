#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_video.h"
#include "entt/entity/fwd.hpp"
#include "fastgltf/types.hpp"
#include "glm/fwd.hpp"
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

void handleEditorCameraMovement(glm::vec3 &eye, glm::vec3 &center, glm::vec3 &up) {
	ImGuiIO &io = ImGui::GetIO();
	const float camera_speed = 0.05f; // adjust accordingly
	if (ImGui::IsKeyDown(ImGuiKey_W)) {
		eye += camera_speed * glm::normalize(center - eye);
		center += camera_speed * glm::normalize(center - eye);
	}
	if (ImGui::IsKeyDown(ImGuiKey_S)) {
		eye -= camera_speed * glm::normalize(center - eye);
		center -= camera_speed * glm::normalize(center - eye);
	}
	if (ImGui::IsKeyDown(ImGuiKey_A)) {
		glm::vec3 right = glm::normalize(glm::cross(center - eye, up));
		eye -= right * camera_speed;
		center -= right * camera_speed;
	}
	if (ImGui::IsKeyDown(ImGuiKey_D)) {
		glm::vec3 right = glm::normalize(glm::cross(center - eye, up));
		eye += right * camera_speed;
		center += right * camera_speed;
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

void drawRenderResult(const Scene &scene,
		const interface::Texture &render_target) {
	ImGui::Begin("render-result");
	ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
	auto content_region_avail = ImGui::GetContentRegionAvail();
	auto &camera = scene.nodes.get<interface::Camera>(scene.active_camera_node);
	if (camera.isOrthogonal()) {
		const_cast<interface::Camera &>(camera).setAspectRatio(
				content_region_avail.x / content_region_avail.y); //! const correctness
	}
	ImGui::Image(
			static_cast<ImTextureID>(interface::Renderer::getTexture(render_target)),
			content_region_avail);
	ImGui::PopStyleVar();
	ImGui::End();
}

void drawRenderOptions(RendererOptions &options) {
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

void drawTexturePreview(const interface::Texture &texture,
		const std::string &texture_name) {
	auto tex_ref = interface::Renderer::getTexture(texture);
	uint32_t preview_width = 50;
	uint32_t preview_height = 50;
	ImGui::Text((texture_name + ": %dx%d").c_str(), texture.getWidth(),
			texture.getHeight());
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
		} else if (region_x > texture.getWidth() - region_sz) {
			region_x = texture.getWidth() - region_sz;
		}
		if (region_y < 0.0f) {
			region_y = 0.0f;
		} else if (region_y > texture.getHeight() - region_sz) {
			region_y = texture.getHeight() - region_sz;
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

void drawComponent(interface::Mesh &component) {
	ImGui::Text("Vert count: %d Index count: %d", component.getVertCount(),
			component.getIndexCount());
}

void drawComponent(interface::Material &component) {
	float metallic_factor = component.getMetallicFactor();
	float roughness_factor = component.getRoughnessFactor();
	float normal_scale = component.getNormalScale();
	glm::vec4 color_factor = component.getColorFactor();
	glm::vec3 emissive_factor = component.getEmissiveFactor();
	if (ImGui::InputFloat("Metallic Factor", &metallic_factor)) {
		component.setMetallicFactor(metallic_factor);
	}
	if (ImGui::InputFloat("Roughness Factor", &roughness_factor)) {
		component.setRoughnessFactor(roughness_factor);
	}
	if (ImGui::InputFloat("Normal Scale", &normal_scale)) {
		component.setMetallicFactor(normal_scale);
	}
	if (ImGui::InputFloat4("Color Factor", glm::value_ptr(color_factor))) {
		component.setColorFactor(color_factor);
	}
	if (ImGui::InputFloat3("Emissive Factor", glm::value_ptr(emissive_factor))) {
		component.setEmissiveFactor(emissive_factor);
	}
	if (component.getColorTexture().isValid()) {
		drawTexturePreview(component.getColorTexture(), "Color Texture");
	}
	if (component.getEmissiveTexture().isValid()) {
		drawTexturePreview(component.getEmissiveTexture(), "Emissive Texture");
	}
	if (component.getNormalTexture().isValid()) {
		drawTexturePreview(component.getNormalTexture(), "Normal Texture");
	}
	if (component.getOcclusionTexture().isValid()) {
		drawTexturePreview(component.getOcclusionTexture(), "Occlusion Texture");
	}
	if (component.getMetallicRoughness().isValid()) {
		drawTexturePreview(component.getMetallicRoughness(),
				"Metallic Roughness Texture");
	}
}

void drawComponent(interface::Camera &component) {
	if (component.isOrthogonal()) {
		float xmag = component.getXMag();
		float ymag = component.getYMag();
		if (ImGui::InputFloat("XMag", &xmag)) {
			component.setXMag(xmag);
		}
		if (ImGui::InputFloat("YMag", &ymag)) {
			component.setYMag(ymag);
		}
	} else {
		float fov = component.getFOV();
		float aspect_ratio = component.getAspectRatio();
		if (ImGui::InputFloat("FOV", &fov)) {
			component.setXMag(fov);
		}
		if (ImGui::InputFloat("Aspect Ratio", &aspect_ratio)) {
			component.setYMag(aspect_ratio);
		}
	}
	float near_plane = component.getNearPlane();
	float far_plane = component.getFarPlane();
	if (ImGui::InputFloat("Near plane", &near_plane)) {
		component.setXMag(near_plane);
	}
	if (ImGui::InputFloat("Far plane", &far_plane)) {
		component.setYMag(far_plane);
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
		if (scene.nodes.any_of<interface::Mesh>(active_node) and
				not ImGui::CollapsingHeader("Mesh")) {
			drawComponent(scene.nodes.get<interface::Mesh>(active_node));
		}
		if (scene.nodes.any_of<interface::Camera>(active_node) and
				not ImGui::CollapsingHeader("Camera")) {
			drawComponent(scene.nodes.get<interface::Camera>(active_node));
		}
		ImGui::End();
	}
	ImGui::End();
}