#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cstdint>
#define CLAY_IMPLEMENTATION

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
#include <misc/log.hpp>
#include <physics/physics-engine.hpp>
#include <renderer/renderer.hpp>
#include <ui/engine-ui.hpp>

struct AppContext {
  Texture render_target;
  SDL_Window *window;
  Renderer *renderer;
  AudioEngine *audio_engine;
  PhysicsEngine *physics_engine;
  EngineUI *engine_ui;
  SDL_AppResult app_status = SDL_APP_CONTINUE;
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

  Renderer *renderer = new Renderer(window);
  AudioEngine *audio_engine = new AudioEngine();
  PhysicsEngine *physics_engine = new PhysicsEngine();
  EngineUI *engine_ui = new EngineUI();

  // set up the application data
  *app_state = new AppContext{
      .render_target = renderer->createTexture(1960, 1080),
      .window = window,
      .renderer = renderer,
      .audio_engine = audio_engine,
      .physics_engine = physics_engine,
      .engine_ui = engine_ui,
  };

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
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = renderer->getGPUDevice();
    init_info.ColorTargetFormat =
        SDL_GetGPUSwapchainTextureFormat(init_info.Device, window);
    init_info.MSAASamples =
        SDL_GPU_SAMPLECOUNT_1; // Only used in multi-viewports mode.
    init_info.SwapchainComposition =
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);
  }

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

  app->renderer->drawToTexture(&app->render_target);

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
  {
    ImGui::Begin("test");
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
  }
  {
    ImGui::Begin("Camera");
    ImGui::SliderFloat("FOV: ", &app->renderer->camera.fov, 0.0f, 100.0f);
    ImGui::SliderFloat("Near plane: ", &app->renderer->camera.nearPlane, 0.0f,
                       100.0f);
    ImGui::SliderFloat("Far plane: ", &app->renderer->camera.farPlane, 0.0f,
                       100.0f);
    ImGui::SliderFloat3("Target", glm::value_ptr(app->renderer->camera.target),
                        -100.0f, 100.0f);
    ImGui::SliderFloat3("Up", glm::value_ptr(app->renderer->camera.up), -100.0f,
                        100.0f);
    ImGui::SliderFloat3("Position",
                        glm::value_ptr(app->renderer->camera.position), -100.0f,
                        100.0f);
    if (ImGui::IsKeyPressed(ImGuiKey_X)) {
      app->renderer->camera.position =
          app->renderer->camera.position -
          glm::normalize(app->renderer->camera.target -
                         app->renderer->camera.position);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
      app->renderer->camera.position =
          app->renderer->camera.position +
          glm::normalize(app->renderer->camera.target -
                         app->renderer->camera.position);
    }
    if (ImGui::IsMouseDragging(2)) {
      ImVec2 drag_delta = ImGui::GetMouseDragDelta(2);
      glm::vec3 forward = glm::normalize(app->renderer->camera.target -
                                         app->renderer->camera.position);
      glm::vec3 W = -forward;
      glm::vec3 U = glm::normalize(glm::cross(app->renderer->camera.up, W));
      glm::vec3 V = glm::normalize(glm::cross(W, U));
      float movement_factor = 0.01f;
      app->renderer->camera.position = app->renderer->camera.position -
                                       movement_factor * U * drag_delta.x +
                                       movement_factor * V * drag_delta.y;
      app->renderer->camera.target = app->renderer->camera.target -
                                     movement_factor * U * drag_delta.x +
                                     movement_factor * V * drag_delta.y;
    }
    ImGui::End();
  }

  {
    ImGui::Begin("render-result");
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
    ImGui::Image(static_cast<ImTextureID>(app->render_target.opq_handle),
                 ImGui::GetContentRegionAvail());
    ImGui::PopStyleVar();
    ImGui::End();
  }
  // ImGui::ShowDemoWindow();
  // ImGui::ShowMetricsWindow();

  // Rendering
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  const bool is_minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(
      app->renderer->getGPUDevice()); // Acquire a GPU command buffer

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
    app->renderer->destroyTexture(&app->render_target);
    delete app->renderer;
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
