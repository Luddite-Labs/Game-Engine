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
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <optional>
#include <queue>
#include <renderer/storage/material-storage.hpp>
#include <renderer/storage/mesh-storage.hpp>
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
	float camera_speed = 5.0f;
	float rot_speed = 50.0f;
	glm::mat4x4 camera_transform = glm::translate(glm::mat4(1), trs.translate) * glm::mat4_cast(trs.rotate) * glm::scale(glm::mat4(1), trs.scale);
	auto view_matrix = glm::inverse(camera_transform);
	glm::vec3 forward_world = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 right_world = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up_world = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 camera_forward_world = glm::normalize(camera_transform * glm::vec4(forward_world, 0.0f));
	glm::vec3 camera_right_world = glm::normalize(camera_transform * glm::vec4(right_world, 0.0f));
	glm::vec3 camera_up_world = glm::normalize(camera_transform * glm::vec4(up_world, 0.0f));
	glm::vec3 forward_world_camera = glm::normalize(view_matrix * glm::vec4(forward_world, 0.0f));
	glm::vec3 right_world_camera = glm::normalize(view_matrix * glm::vec4(right_world, 0.0f));
	glm::vec3 up_world_camera = glm::normalize(view_matrix * glm::vec4(up_world, 0.0f));

	// WASD for movement
	if (io.MouseWheel) {
		trs.translate += camera_forward_world * io.MouseWheel; // invert
	}
	if (io.MouseDown[ImGuiMouseButton_Middle]) {
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			//! better solution needed

			float delta_x = camera_speed * (io.MouseDelta.x / content_region.x) * RE::Camera::getAspectRatio(camera.handle) * -1;
			float delta_y = camera_speed * (io.MouseDelta.y / content_region.y); // invert
			trs.translate += delta_x * camera_right_world + delta_y * camera_up_world;
		}
		// Middle mouse button for rotation
		else {
			float delta_x = rot_speed * (io.MouseDelta.x / content_region.x) * -1; // invert
			float delta_y = rot_speed * (io.MouseDelta.y / content_region.y) * -1; // invert
			if (std::abs(delta_x) > 0.1) {
				trs.rotate *= glm::angleAxis(glm::radians(delta_x), up_world_camera);
			}
			if (std::abs(delta_y) > 0.1) {
				trs.rotate *= glm::angleAxis(glm::radians(delta_y), right_world);
			}
		}
	}
	trs.rotate = glm::normalize(trs.rotate);
}

std::string drawDebugViews(std::string selected_view) {
	std::vector<std::string> available_views = getRegisteredUIDebugCallbacks();
	ImGui::Begin("Debug View");
	if (ImGui::BeginCombo("Available Views", selected_view.c_str())) {
		for (auto &view : available_views) {
			if (ImGui::Selectable(view.c_str(), selected_view == view)) {
				selected_view = view;
			}
		}
		ImGui::EndCombo();
	}
	if (selected_view != "") {
		getUIDebugCallbackByName(selected_view)();
	}
	ImGui::End();
	return selected_view;
}

SDL_DialogFileFilter file_filters = { .name = "GLTF File",
	.pattern = "gltf" };
void drawToolBar(SDL_Window *window) {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::BeginMenu("Import...")) {
				if (ImGui::MenuItem(".gltf")) {
					SDL_ShowOpenFileDialog(loadGLTFCallback, nullptr, window, &file_filters,
							1, nullptr, false);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void drawStatusBar(SDL_Window *window) {
	float windowHeight = 25.0f;
	ImGuiViewport *viewport = ImGui::GetMainViewport();

	if (ImGui::BeginViewportSideBar("##Status-Bar", viewport, ImGuiDir_Down, windowHeight, ImGuiWindowFlags_NoScrollbar)) {
		const std::string status_message = getStatusMessage();
		ImGui::TextUnformatted((status_message.substr(0, 20) + ((status_message.length() > 20) ? "..." : "")).c_str());
		ImGui::Separator();
		ImGui::End();
	}
}

glm::vec4 drawRenderResult(
		const RE::Texture::Shared &render_target, Transform &trs, RE::Camera::Shared camera) {
	ImGui::Begin("render-result", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
	auto content_region_avail = ImGui::GetContentRegionAvail();
	auto content_min = ImGui::GetCursorScreenPos();
	auto content_max = ImVec2{ content_min.x + content_region_avail.x, content_min.y + content_region_avail.y };
	ImGui::GetWindowDrawList()->AddImage(
			static_cast<ImTextureID>(RE::getTexture(render_target.handle)),
			content_min,
			content_max);
	RE::Camera::setAspectRatio(camera.handle, content_region_avail.x / content_region_avail.y);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
	ImGuizmo::SetRect(content_min.x, content_min.y, content_region_avail.x, content_region_avail.y);
	auto view = glm::inverse(glm::translate(glm::mat4(1), trs.translate) * glm::mat4_cast(trs.rotate) * glm::scale(glm::mat4(1), trs.scale));
	auto proj = RE::getProjectionMatrix(camera.handle);
	auto model = glm::mat4(1.0f);
	ImGuizmo::ViewManipulate(
			glm::value_ptr(view),
			8.f,
			content_min,
			ImVec2(128, 128),
			0x10101010);
	if (ImGuizmo::IsUsingViewManipulate()) {
		glm::vec3 s;
		glm::quat r;
		glm::vec3 t;
		glm::vec3 sk;
		glm::vec4 p;
		glm::decompose(glm::inverse(view), s, r, t, sk, p);
		trs.translate = t;
		trs.scale = s;
		trs.rotate = glm::normalize(r);
	}

	if (ImGui::IsWindowHovered()) {
		handleEditorCameraMovement(trs, camera, glm::vec2(content_region_avail.x, content_region_avail.y));
	}
	ImGui::PopStyleVar();
	ImGui::End();
	return { content_min.x, content_min.y, content_region_avail.x, content_region_avail.y };
}

void drawRenderOptions(RE::Options &options) {
	ImGuiIO &io = ImGui::GetIO();
	ImGui::Begin("Renderer Options");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / io.Framerate, io.Framerate);
	ImGui::ColorEdit4("Clear Color:",
			glm::value_ptr(options.clear_color));
	if (ImGui::BeginCombo("Layers", getString(options.layers))) {
		if (ImGui::Selectable(getString(RE::Layers::NONE), options.layers == RE::Layers::NONE)) {
			options.layers = RE::Layers::NONE;
		}
		if (ImGui::Selectable(getString(RE::Layers::ALBEDO), options.layers == RE::Layers::ALBEDO)) {
			options.layers = RE::Layers::ALBEDO;
		}
		if (ImGui::Selectable(getString(RE::Layers::VERTEX_NORMAL), options.layers == RE::Layers::VERTEX_NORMAL)) {
			options.layers = RE::Layers::VERTEX_NORMAL;
		}
		if (ImGui::Selectable(getString(RE::Layers::NORMAL_TEXTURE), options.layers == RE::Layers::NORMAL_TEXTURE)) {
			options.layers = RE::Layers::NORMAL_TEXTURE;
		}
		if (ImGui::Selectable(getString(RE::Layers::SHADING_NORMAL), options.layers == RE::Layers::SHADING_NORMAL)) {
			options.layers = RE::Layers::SHADING_NORMAL;
		}
		if (ImGui::Selectable(getString(RE::Layers::METALLIC), options.layers == RE::Layers::METALLIC)) {
			options.layers = RE::Layers::METALLIC;
		}
		if (ImGui::Selectable(getString(RE::Layers::ROUGHNESS), options.layers == RE::Layers::ROUGHNESS)) {
			options.layers = RE::Layers::ROUGHNESS;
		}
		if (ImGui::Selectable(getString(RE::Layers::EMISSIVE), options.layers == RE::Layers::EMISSIVE)) {
			options.layers = RE::Layers::EMISSIVE;
		}
		if (ImGui::Selectable(getString(RE::Layers::OCCLUSION), options.layers == RE::Layers::OCCLUSION)) {
			options.layers = RE::Layers::OCCLUSION;
		}
		if (ImGui::Selectable(getString(RE::Layers::SPECULAR), options.layers == RE::Layers::SPECULAR)) {
			options.layers = RE::Layers::SPECULAR;
		}
		if (ImGui::Selectable(getString(RE::Layers::DIFFUSE), options.layers == RE::Layers::DIFFUSE)) {
			options.layers = RE::Layers::DIFFUSE;
		}
		if (ImGui::Selectable(getString(RE::Layers::DIELECTRIC), options.layers == RE::Layers::DIELECTRIC)) {
			options.layers = RE::Layers::DIELECTRIC;
		}
		if (ImGui::Selectable(getString(RE::Layers::METALLIC_BRDF), options.layers == RE::Layers::METALLIC_BRDF)) {
			options.layers = RE::Layers::METALLIC_BRDF;
		}
		if (ImGui::Selectable(getString(RE::Layers::TANGENT), options.layers == RE::Layers::TANGENT)) {
			options.layers = RE::Layers::TANGENT;
		}
		if (ImGui::Selectable(getString(RE::Layers::UV), options.layers == RE::Layers::UV)) {
			options.layers = RE::Layers::UV;
		}
		if (ImGui::Selectable(getString(RE::Layers::DEPTH), options.layers == RE::Layers::DEPTH)) {
			options.layers = RE::Layers::DEPTH;
		}
		if (ImGui::Selectable(getString(RE::Layers::VERTEX_COLOR), options.layers == RE::Layers::VERTEX_COLOR)) {
			options.layers = RE::Layers::VERTEX_COLOR;
		}
		if (ImGui::Selectable(getString(RE::Layers::ALPHA), options.layers == RE::Layers::ALPHA)) {
			options.layers = RE::Layers::ALPHA;
		}

		ImGui::EndCombo();
	}
	ImGui::End();
}

void drawComponent(Transform &component, Transform camera_transform, glm::mat4 global_parent_transform, RE::Camera::Shared camera) {
	auto view = glm::inverse(glm::translate(glm::mat4(1), camera_transform.translate) * glm::mat4_cast(camera_transform.rotate) * glm::scale(glm::mat4(1), camera_transform.scale));
	auto proj = RE::getProjectionMatrix(camera.handle);

	auto obj = glm::translate(glm::mat4(1.f), component.translate) * glm::mat4_cast(component.rotate) * glm::scale(glm::mat4(1.f), component.scale);
	auto global_transform = global_parent_transform * obj;

	auto isMatrixFinite = [](const glm::mat4 &m) {
		for (int c = 0; c < 4; ++c) {
			for (int r = 0; r < 4; ++r) {
				float v = m[c][r];
				if (!std::isfinite(v)) {
					return false;
				}
			}
		}
		return true;
	};

	if (!isMatrixFinite(global_parent_transform) || !isMatrixFinite(obj) || !isMatrixFinite(global_transform)) {
		LOG_INFO("ImGuizmo: matrix contains NaN/Inf — skipping Manipulate");
		return;
	}

	ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE, ImGuizmo::MODE::LOCAL, glm::value_ptr(global_transform));

	if (ImGuizmo::IsUsing()) {
		const float MIN_SCALE = 1e-6f;

		// Decompose parent transform
		glm::vec3 parent_trans(global_parent_transform[3][0], global_parent_transform[3][1], global_parent_transform[3][2]);
		glm::vec3 parent_basis_x(global_parent_transform[0]);
		glm::vec3 parent_basis_y(global_parent_transform[1]);
		glm::vec3 parent_basis_z(global_parent_transform[2]);
		float parent_scale_x = glm::length(parent_basis_x);
		float parent_scale_y = glm::length(parent_basis_y);
		float parent_scale_z = glm::length(parent_basis_z);
		if (parent_scale_x < MIN_SCALE) {
			parent_scale_x = MIN_SCALE;
		}
		if (parent_scale_y < MIN_SCALE) {
			parent_scale_y = MIN_SCALE;
		}
		if (parent_scale_z < MIN_SCALE) {
			parent_scale_z = MIN_SCALE;
		}
		glm::vec3 parent_scale(parent_scale_x, parent_scale_y, parent_scale_z);
		glm::quat parent_rot = glm::normalize(glm::quat_cast(glm::mat3(
				parent_basis_x / parent_scale_x,
				parent_basis_y / parent_scale_y,
				parent_basis_z / parent_scale_z)));

		// Decompose global transform
		glm::vec3 global_trans(global_transform[3][0], global_transform[3][1], global_transform[3][2]);
		glm::vec3 global_basis_x(global_transform[0]);
		glm::vec3 global_basis_y(global_transform[1]);
		glm::vec3 global_basis_z(global_transform[2]);
		float global_scale_x = glm::length(global_basis_x);
		float global_scale_y = glm::length(global_basis_y);
		float global_scale_z = glm::length(global_basis_z);
		if (global_scale_x < MIN_SCALE) {
			global_scale_x = MIN_SCALE;
		}
		if (global_scale_y < MIN_SCALE) {
			global_scale_y = MIN_SCALE;
		}
		if (global_scale_z < MIN_SCALE) {
			global_scale_z = MIN_SCALE;
		}
		glm::vec3 global_scale(global_scale_x, global_scale_y, global_scale_z);
		glm::quat global_rot = glm::normalize(glm::quat_cast(glm::mat3(
				global_basis_x / global_scale_x,
				global_basis_y / global_scale_y,
				global_basis_z / global_scale_z)));

		// Extract local by "dividing out" parent components (no inverse needed)
		glm::quat parent_rot_inv = glm::conjugate(parent_rot);
		glm::vec3 new_translate = parent_rot_inv * (global_trans - parent_trans);
		new_translate = glm::vec3(
				new_translate.x / parent_scale.x,
				new_translate.y / parent_scale.y,
				new_translate.z / parent_scale.z);
		glm::quat new_rotate = parent_rot_inv * global_rot;
		glm::vec3 new_scale = glm::vec3(
				global_scale.x / parent_scale.x,
				global_scale.y / parent_scale.y,
				global_scale.z / parent_scale.z);

		component.translate = new_translate;
		component.rotate = glm::normalize(new_rotate);
		component.scale = new_scale;
	}

	bool edit = false;
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(component.rotate));
	glm::vec3 editableEulerAngles = eulerAngles;
	ImGui::InputFloat3("Translate", glm::value_ptr(component.translate));
	if (ImGui::SliderFloat3("Rotate", glm::value_ptr(editableEulerAngles), -360.0f, 360.0f)) {
		auto comp = glm::epsilonNotEqual(eulerAngles, editableEulerAngles, glm::vec3{ 0.01 });
		if (glm::any(comp)) {
			component.rotate = glm::quat_cast(glm::eulerAngleXYZ(
					editableEulerAngles.x,
					editableEulerAngles.y,
					editableEulerAngles.z));
		}
	}
	ImGui::SetItemTooltip("Quat: %f %f %f %f", component.rotate.x, component.rotate.y, component.rotate.z, component.rotate.w);
	ImGui::InputFloat3("Scale", glm::value_ptr(component.scale));
}

void drawComponent(RE::Material::Handle &component) {
	MaS::drawMaterialDebugUI(component);
}

void drawComponent(RE::Mesh::Handle &component) {
	ImGui::Indent();
	MS::drawMeshDebugUI(component);
	ImGui::Unindent();
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
	ImGuiTreeNodeFlags node_flags = base_flags;
	if (scene.nodes.any_of<fastgltf::MaybeSmallVector<Child>>(node)) {
		children = scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(node);
	}
	if (not children.has_value()) {
		node_flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (active_node == node) {
		node_flags |= ImGuiTreeNodeFlags_Selected;
	}
	std::string node_name;
	if (scene.nodes.any_of<Tag>(node)) {
		node_name = scene.nodes.get<Tag>(node).name +
				std::to_string(static_cast<uint32_t>(node));
	} else {
		node_name = "Untitled##" + std::to_string(static_cast<uint32_t>(node));
	}

	if (ImGui::TreeNodeEx(node_name.c_str(), node_flags)) {
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
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

void drawSceneGraph(SceneManager *scene_manager, Transform camera_transform, RE::Camera::Shared camera) {
	if (0 > scene_manager->active_scene_index ||
			scene_manager->active_scene_index >= scene_manager->scenes.size()) {
		return;
	}

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
			ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_OpenOnDoubleClick;
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
			glm::mat4 global_parent_transform = glm::mat4(1.f);
			entt::entity parent = entt::null;
			if (scene.nodes.any_of<Parent>(active_node)) {
				parent = scene.nodes.get<Parent>(active_node).node;
			}
			while (parent != entt::null) {
				Transform &parent_transform = scene.nodes.get<Transform>(parent);
				glm::mat4 parent_transform_mat = glm::translate(glm::mat4(1.f), parent_transform.translate) * glm::mat4_cast(parent_transform.rotate) * glm::scale(glm::mat4(1.f), parent_transform.scale);
				global_parent_transform = parent_transform_mat * global_parent_transform;
				if (scene.nodes.any_of<Parent>(parent)) {
					parent = scene.nodes.get<Parent>(parent).node;
				} else {
					parent = entt::null;
				}
			}
			drawComponent(scene.nodes.get<Transform>(active_node), camera_transform, global_parent_transform, camera);
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