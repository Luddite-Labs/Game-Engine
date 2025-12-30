
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "renderer/common.hpp"
#include "renderer/interface.hpp"
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
#include <ui/engine-ui.hpp>

#include <glm/vec3.hpp>

struct AppContext {
  interface::Texture *render_target;
  SDL_Window *window;
  AudioEngine *audio_engine;
  PhysicsEngine *physics_engine;
  EngineUI *engine_ui;
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

  Renderer::init();
  SceneManager::init();
  AudioEngine *audio_engine = new AudioEngine();
  PhysicsEngine *physics_engine = new PhysicsEngine();
  EngineUI *engine_ui = new EngineUI();

  // set up the application data
  *app_state = new AppContext{
      .render_target = new interface::Texture(1920, 1080,
                                              TextureUsageFlags::COLOR_TARGET |
                                                  TextureUsageFlags::SAMPLER),
      .window = window,
      .audio_engine = audio_engine,
      .physics_engine = physics_engine,
      .engine_ui = engine_ui,
      .eye = {2.5, 0, 1.0},
      .center = {0.0, 0.0, 0.0},
      .up = {0.0, 0.0, 1.0}};
  load(GAME_ENGINE_DEFAULT_DATA_DIR "/scenes/shiba/");

  LOG_INFO("Application started successfully!");

  {
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable
    // Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(
        main_scale); // Bake a fixed style scale. (until we have a solution for
                     // dynamic style scaling, changing this requires resetting
                     // Style + calling this again)
    style.FontScaleDpi =
        main_scale; // Set initial font scale. (using
                    // io.ConfigDpiScaleFonts=true makes this unnecessary. We
                    // leave both here for documentation purpose)
    io.ConfigDpiScaleFonts =
        true; // [Experimental] Automatically overwrite style.FontScaleDpi in
              // Begin() when Monitor DPI changes. This will scale fonts but
              // _NOT_ scale sizes/padding for now.
    // io.ConfigDpiScaleViewports =
    //     true; // [Experimental] Scale Dear ImGui and Platform Windows when
    // Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    SDL_ClaimWindowForGPUDevice(Renderer::getSingleton()->getGPUDevice(),
                                window); //! remove once imgui backend changed
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = Renderer::getSingleton()->getGPUDevice();
    init_info.ColorTargetFormat =
        SDL_GetGPUSwapchainTextureFormat(init_info.Device, window);
    init_info.MSAASamples =
        SDL_GPU_SAMPLECOUNT_1; // Only used in multi-viewports mode.
    init_info.SwapchainComposition =
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);
  }
  reinterpret_cast<AppContext *>(app_state)->prev_mouse_cursor =
      ImGui::GetIO().MouseWheel;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *app_state, SDL_Event *event) {
  auto *app = reinterpret_cast<AppContext *>(app_state);
  // return app->engine_ui->processEvent(event);
  ImGui_ImplSDL3_ProcessEvent(event);
  if ((event->type == SDL_EVENT_QUIT) ||
      (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
       event->window.windowID == SDL_GetWindowID(app->window))) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *app_state) {
  auto *app = reinterpret_cast<AppContext *>(app_state);

  const auto &scene = SceneManager::getSingleton()->scenes[0];
  std::vector<interface::DrawCommand> draw_commands;
  for (auto &node : scene.nodes) {
    draw_commands.push_back({
        node.mesh,
        node.material,
        node.transform,
    });
  }
  interface::Renderer::draw(
      *app->render_target, scene.cameras[scene.active_camera_index],
      glm::lookAt(app->eye, app->center, app->up), draw_commands);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  if (SDL_GetWindowFlags(app->window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return app->app_status;
  }

  // Start the Dear ImGui frame
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair
  // to create a named window.

  ImGui::DockSpaceOverViewport();
  ImGuiIO &io = ImGui::GetIO();

  float render_width = 0.0f;
  {
    ImGui::Begin("render-result");
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
    auto content_region_avail = ImGui::GetContentRegionAvail();
    render_width = content_region_avail.x;
    ImGui::Image(static_cast<ImTextureID>(
                     interface::Renderer::getTexture(*app->render_target)),
                 content_region_avail);
    ImGui::PopStyleVar();
    ImGui::End();
  }

  {
    ImGui::Begin("test");
    ImGui::ColorPicker4("Clear Color:",
                        glm::value_ptr(Renderer::getSingleton()->clear_color));
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
  }
  {
    ImGui::Begin("Camera");
    ImGui::InputFloat3("Position", glm::value_ptr(app->eye));
    ImGui::InputFloat3("Target", glm::value_ptr(app->center));
    ImGui::InputFloat3("Up", glm::value_ptr(app->up));
    ImGui::End();
  }

  ImGui::ShowDemoWindow();
  // ImGui::ShowMetricsWindow();

  // Rendering
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  const bool is_minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(
      Renderer::getSingleton()->getGPUDevice()); // Acquire a GPU command buffer

  SDL_GPUTexture *swapchain_texture;
  SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app->window,
                                        &swapchain_texture, nullptr,
                                        nullptr); // Acquire a swapchain texture

  if (swapchain_texture != nullptr && !is_minimized) {
    // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the
    // vertex/index buffer!
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

    // Setup and start a render pass
    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapchain_texture;
    target_info.clear_color =
        SDL_FColor{clear_color.x, clear_color.y, clear_color.z, clear_color.w};
    target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    target_info.mip_level = 0;
    target_info.layer_or_depth_plane = 0;
    target_info.cycle = false;
    SDL_GPURenderPass *render_pass =
        SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

    // Render ImGui
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

    SDL_EndGPURenderPass(render_pass);
  }

  // Update and Render additional Platform Windows
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  // Submit the command buffer
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return app->app_status;
}

void SDL_AppQuit(void *app_state, SDL_AppResult result) {
  auto *app = reinterpret_cast<AppContext *>(app_state);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();
  if (app) {
    delete app->render_target;
    Renderer::destroy();
    SceneManager::destroy();
    delete app->audio_engine;
    delete app->physics_engine;
    delete app->engine_ui;
    SDL_DestroyWindow(app->window);
    delete app;
  }
  TTF_Quit();

  LOG_INFO("Application quit successfully!");
  SDL_Quit();
}
