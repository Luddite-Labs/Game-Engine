
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "renderer/types.hpp"
#include "renderer/wrappers.hpp"
#include "scene/components.hpp"
#include "scene/scene-manager.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <glm/common.hpp>

#include <cmath>

#include <audio/audio-engine.hpp>
#include <engine/engine.hpp>
#include <loaders/gltf-loader.hpp>
#include <misc/log.hpp>
#include <physics/physics-engine.hpp>
#include <renderer/renderer.hpp>
#include <ui/ui.hpp>

#include <glm/vec3.hpp>
#include <ui/editor-ui.hpp>

glm::mat4x4 getTransformMatFromTRS(const Transform &trs) {
	return glm::translate(glm::mat4(1.0f), trs.translate) *
			glm::mat4_cast(trs.rotate) *
			glm::scale(glm::mat4(1.0f), trs.scale);
}

void traverseRootNode(Scene &scene, entt::entity node,
		const glm::mat4 &parent_transform,
		std::vector<RE::DrawCommand> &draw_commands) {
	if (scene.nodes.all_of<Disabled>(node)) {
		return;
	}
	glm::mat4 local_transform = glm::mat4(1.0f);
	if (scene.nodes.all_of<Transform>(node)) {
		const auto &trs = scene.nodes.get<Transform>(node);
		local_transform = getTransformMatFromTRS(trs);
	}
	glm::mat4 global_transform = parent_transform * local_transform;

	if (scene.nodes.all_of<RenderableMesh>(node)) {
		auto &renderable = scene.nodes.get<RenderableMesh>(node);
		draw_commands.emplace_back(RE::CommandType::Mesh, global_transform, renderable.mesh.handle);
	}

	if (scene.nodes.all_of<fastgltf::MaybeSmallVector<Child>>(node)) {
		auto &children_comp = scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(node);
		for (auto child : children_comp) {
			traverseRootNode(scene, child.node, global_transform, draw_commands);
		}
	}
}

struct AppContext {
	RE::Texture::Shared render_target;
	RE::Options renderer_options;
	RE::Camera::Shared scene_camera;
	Transform camera_transform;
	SDL_AppResult app_status = SDL_APP_CONTINUE;
	SDL_Window *window;
};

SDL_AppResult SDL_Fail() {
	LOG_ERROR("Error %s", SDL_GetError());
	return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **app_state, int argc, char *argv[]) {
	util::setFileLogging(GAME_ENGINE_BUILD_DIR "debug.log", true);
	if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		return SDL_Fail();
	}

	if (not TTF_Init()) {
		return SDL_Fail();
	}

	SDL_Window *window = SDL_CreateWindow(
			"Window", 800, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (not window) {
		return SDL_Fail();
	}

	// print some information about the window
	SDL_ShowWindow(window);
	{
		int width, height, bb_width, bb_height;
		SDL_GetWindowSize(window, &width, &height);
		SDL_GetWindowSizeInPixels(window, &bb_width, &bb_height);
		LOG_INFO("Window size: %ix%i", width, height);
		LOG_INFO("Backbuffer size: %ix%i", bb_width, bb_height);
		if (width != bb_width) {
			LOG_INFO("This is a highdpi environment.");
		}
	}

	RE::init();
	UI::init(window);
	SceneManager::init();
	AudioEngine *audio_engine = new AudioEngine();
	PhysicsEngine *physics_engine = new PhysicsEngine();

	RE::Camera::Shared scene_camera{ RE::Camera::create() };
	RE::Camera::setAspectRatio(scene_camera.handle, 1.77);
	RE::Camera::setFOV(scene_camera.handle, glm::radians(75.0f));
	RE::Camera::setNearPlane(scene_camera.handle, 1.0f);
	RE::Camera::setFarPlane(scene_camera.handle, 100.0f);

	*app_state = new AppContext{
		.render_target = RE::Texture::Shared{ RE::Texture::create(1920, 1080,
				RE::Texture::UsageFlags::COLOR_TARGET |
						RE::Texture::UsageFlags::SAMPLER,
				RE::Texture::Format::R8G8B8A8_UNORM) }, //! not srgb since imgui renders incorrectly
		.renderer_options =
				RE::Options{ .clear_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) },
		.scene_camera = scene_camera,
		.window = window,
	};
	auto &camera_transform = static_cast<AppContext *>(*app_state)->camera_transform;
	camera_transform.translate = { -10, 5, 10 };
	auto dir = glm::normalize(camera_transform.translate);
	camera_transform.rotate = glm::quatLookAt(-dir, glm::vec3{ 0, 1, 0 });
	camera_transform.scale = glm::vec3{ 1, 1, 1 };
	util::setFileLogging(GAME_ENGINE_BUILD_DIR "debug.log", true);
	LOG_INFO("Application started successfully!");
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *app_state, SDL_Event *event) {
	auto *app = reinterpret_cast<AppContext *>(app_state);
	UI::getSingleton()->processEvent(event);

	if ((event->type == SDL_EVENT_QUIT) ||
			(event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
					event->window.windowID == SDL_GetWindowID(app->window))) {
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *app_state) {
	auto *app = reinterpret_cast<AppContext *>(app_state);
	if (SDL_GetWindowFlags(app->window) & SDL_WINDOW_MINIMIZED) {
		SDL_Delay(10);
		return app->app_status;
	}
	SceneManager *scene_manager = SceneManager::getSingleton();

	UI::getSingleton()->beginFrame();
	drawToolBar(app->window);
	glm::vec2 content_region{};
	drawRenderResult(app->render_target, app->camera_transform, app->scene_camera);
	drawRenderOptions(app->renderer_options);
	drawSceneGraph(scene_manager);
	if (0 <= scene_manager->active_scene_index &&
			scene_manager->active_scene_index < scene_manager->scenes.size()) {
		auto &scene = scene_manager->scenes[scene_manager->active_scene_index];
		std::vector<RE::DrawCommand> draw_commands;
		const auto root_node_view = scene.nodes.view<entt::entity>(entt::exclude<Parent>);
		for (const auto &node : root_node_view) {
			traverseRootNode(scene, node, glm::mat4(1.0f), draw_commands);
		}
		RE::drawToTexture(app->renderer_options, app->render_target.handle,
				app->scene_camera.handle,
				glm::inverse(getTransformMatFromTRS(app->camera_transform)), draw_commands);
	}
	UI::getSingleton()->endFrame(app->window);
	return app->app_status;
}

void SDL_AppQuit(void *app_state, SDL_AppResult result) {
	auto *app = reinterpret_cast<AppContext *>(app_state);
	LOG_INFO("Application destruction started!");
	if (app) {
		app->render_target.reset();
		SceneManager::destroy();
		UI::destroy();
		RE::destroy();
		SDL_DestroyWindow(app->window);
		delete app;
	}
	TTF_Quit();

	LOG_INFO("Application quit successfully!");
	SDL_Quit();
}
