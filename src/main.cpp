
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
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <engine/engine.hpp>
#include <imgui.h>
#include <loaders/gltf-loader.hpp>
#include <misc/log.hpp>
#include <physics/physics-engine.hpp>
#include <renderer/renderer.hpp>
#include <ui/ui.hpp>

#include <glm/vec3.hpp>
#include <ui/editor-ui.hpp>

void traverseRootNode(Scene &scene, entt::entity node,
		const glm::mat4 &parent_transform,
		std::vector<interface::DrawCommand> &draw_commands) {
	if (scene.nodes.all_of<Disabled>(node)) {
		return;
	}
	glm::mat4 local_transform = glm::mat4(1.0f);
	if (scene.nodes.all_of<Transform>(node)) {
		const auto &trs = scene.nodes.get<Transform>(node);
		local_transform = glm::translate(glm::mat4(1.0f), trs.translate) *
				glm::mat4_cast(trs.rotate) *
				glm::scale(glm::mat4(1.0f), trs.scale);
	}
	glm::mat4 global_transform = parent_transform * local_transform;

	if (scene.nodes.all_of<RenderableMesh>(node)) {
		auto &renderable = scene.nodes.get<RenderableMesh>(node);
		draw_commands.emplace_back(renderable.mesh, global_transform);
	}

	if (scene.nodes.all_of<fastgltf::MaybeSmallVector<Child>>(node)) {
		auto &children_comp = scene.nodes.get<fastgltf::MaybeSmallVector<Child>>(node);
		for (auto child : children_comp) {
			traverseRootNode(scene, child.node, global_transform, draw_commands);
		}
	}
}

struct AppContext {
	interface::Texture *render_target;
	SDL_Window *window;
	AudioEngine *audio_engine;
	PhysicsEngine *physics_engine;
	RendererOptions renderer_options;
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
	SDL_AppResult app_status = SDL_APP_CONTINUE;
	bool isFirstFameDragging = true;
	glm::vec2 prev_mouse_position;
	float prev_mouse_cursor;
};

SDL_AppResult SDL_Fail() {
	LOG_ERROR("Error %s", SDL_GetError());
	return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **app_state, int argc, char *argv[]) {
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

	interface::Renderer::init();
	UI::init(window);
	SceneManager::init();
	AudioEngine *audio_engine = new AudioEngine();
	PhysicsEngine *physics_engine = new PhysicsEngine();

	// set up the application data
	*app_state = new AppContext{
		.render_target = new interface::Texture(1920, 1080,
				TextureUsageFlags::COLOR_TARGET |
						TextureUsageFlags::SAMPLER),
		.window = window,
		.audio_engine = audio_engine,
		.physics_engine = physics_engine,
		.renderer_options =
				RendererOptions{ .clear_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) },
		.eye = { 5.0, 0.0, 5.0 },
		.center = { 0.0, 0.0, 0.0 },
		.up = { 0.0, 1.0, 0.0 }
	};

	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/Box/glTF/Box.gltf");
	// // load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");
	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf");
	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/Cube/glTF/Cube.gltf");
	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/BoxVertexColors/glTF/BoxVertexColors.gltf");
	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/BoxTextured/glTF/BoxTextured.gltf");
	// load(GAME_ENGINE_DEFAULT_DATA_DIR "scenes/glTF-Sample-Models/2.0/BoxTexturedNonPowerOfTwo/glTF/BoxTexturedNonPowerOfTwo.gltf");
	LOG_INFO("Application started successfully!");

	reinterpret_cast<AppContext *>(app_state)->prev_mouse_cursor =
			ImGui::GetIO().MouseWheel;
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
	if (0 <= scene_manager->active_scene_index &&
			scene_manager->active_scene_index < scene_manager->scenes.size()) {
		auto &scene = scene_manager->scenes[scene_manager->active_scene_index];
		std::vector<interface::DrawCommand> draw_commands;
		const auto root_node_view = scene.nodes.view<entt::entity>(entt::exclude<Parent>);
		for (const auto &node : root_node_view) {
			traverseRootNode(scene, node, glm::mat4(1.0f), draw_commands);
		}
		interface::Renderer::draw(app->renderer_options, *app->render_target,
				scene.nodes.get<interface::Camera>(scene.active_camera_node),
				glm::lookAt(app->eye, app->center, app->up), draw_commands);
		drawRenderResult(scene, *app->render_target);
	}
	handleEditorCameraMovement(app->eye, app->center, app->up);	
	drawRenderOptions(app->renderer_options);
	drawSceneGraph(scene_manager);
	UI::getSingleton()->endFrame(app->window);
	return app->app_status;
}

void SDL_AppQuit(void *app_state, SDL_AppResult result) {
	auto *app = reinterpret_cast<AppContext *>(app_state);

	if (app) {
		delete app->render_target;
		SceneManager::destroy();
		UI::destroy();
		interface::Renderer::destroy();
		delete app->audio_engine;
		delete app->physics_engine;
		SDL_DestroyWindow(app->window);
		delete app;
	}
	TTF_Quit();

	LOG_INFO("Application quit successfully!");
	SDL_Quit();
}
