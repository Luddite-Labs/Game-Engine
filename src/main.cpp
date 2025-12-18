#include "SDL3/SDL_stdinc.h"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/scalar_relational.hpp"
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

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <glm/vec3.hpp>

struct AppContext {
  Texture render_target;
  SDL_Window *window;
  Renderer *renderer;
  AudioEngine *audio_engine;
  PhysicsEngine *physics_engine;
  EngineUI *engine_ui;
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

  fastgltf::Extensions extensions;
  fastgltf::Parser parser(extensions);
  auto gltfFile = fastgltf::GltfDataBuffer::FromPath(
      GAME_ENGINE_DEFAULT_DATA_DIR "/scenes/shiba/scene.gltf");
  auto asset = parser.loadGltf(gltfFile.get(),
                               GAME_ENGINE_DEFAULT_DATA_DIR "/scenes/shiba/",
                               fastgltf::Options::GenerateMeshIndices |
                                   fastgltf::Options::LoadExternalBuffers);
  std::vector<uint16_t> indices;
  std::vector<Vertex> vertices;
  for (auto &mesh : asset->meshes) {

    indices.clear();
    vertices.clear();
    for (auto &&p : mesh.primitives) {
      size_t initial_vtx = vertices.size();
      // load indexes
      {
        fastgltf::Accessor &indexaccessor =
            asset->accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<uint32_t>(
            asset.get(), indexaccessor, [&](uint32_t idx) {
              indices.push_back(static_cast<uint16_t>(idx + initial_vtx));
            });
      }

      // load vertex positions
      {
        fastgltf::Accessor &posAccessor =
            asset->accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset.get(), posAccessor, [&](glm::vec3 v, size_t index) {
              Vertex newvtx;
              newvtx.position = v;
              newvtx.normal = {1, 0, 0};
              vertices[initial_vtx + index] = newvtx;
            });
      }

      // load UV
      {
        auto at_it = p.findAttribute("TEXCOORD_0");
        if (at_it != p.attributes.end()) {
          fastgltf::Accessor &uvAccessor =
              asset->accessors[at_it->accessorIndex];
          fastgltf::iterateAccessorWithIndex<glm::vec2>(
              asset.get(), uvAccessor, [&](glm::vec2 v, size_t index) {
                vertices[initial_vtx + index].uv = v;
              });
        }
      }
      
      // load vertex normals
      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset.get(), asset->accessors[(*normals).accessorIndex],
            [&](glm::vec3 v, size_t index) {
              vertices[initial_vtx + index].normal = v;
            });
      }
      MeshData mesh_data = {
          .vert_buffer = static_cast<void *>(vertices.data()),
          .index_buffer = static_cast<void *>(indices.data()),
          .vert_count = static_cast<uint32_t>(vertices.size()),
          .index_count = static_cast<uint32_t>(indices.size())};
      renderer->createMesh(&mesh_data);
    }
  }

  LOG_INFO("gltf file size % d", gltfFile->totalSize());

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

  float render_width = 0.0f;
  {
    ImGui::Begin("render-result");
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
    auto content_region_avail = ImGui::GetContentRegionAvail();
    render_width = content_region_avail.x;
    app->renderer->camera.aspectRatio =
        content_region_avail.x / content_region_avail.y;
    ImGui::Image(static_cast<ImTextureID>(app->render_target.opq_handle),
                 content_region_avail);
    ImGui::PopStyleVar();
    ImGui::End();
  }

  {
    ImGui::Begin("test");
    ImGui::ColorPicker4("Clear Color:",
                        glm::value_ptr(app->renderer->clear_color));
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
  }
  {
    ImGui::Begin("Camera");
    ImGui::InputFloat("FOV: ", &app->renderer->camera.fov);
    ImGui::InputFloat("Near plane: ", &app->renderer->camera.nearPlane);
    ImGui::InputFloat("Far plane: ", &app->renderer->camera.farPlane);
    ImGui::InputFloat3("Target", glm::value_ptr(app->renderer->camera.target));
    ImGui::InputFloat3("Up", glm::value_ptr(app->renderer->camera.up));
    ImGui::InputFloat3("Position",
                       glm::value_ptr(app->renderer->camera.position));
    ImGui::Checkbox("Is Orthogonal?", &app->renderer->camera.is_orthogonal);
    float mouse_cursor = ImGui::GetIO().MouseWheel;
    if (mouse_cursor != app->prev_mouse_cursor) {
      float cursor_delta = mouse_cursor - app->prev_mouse_cursor;
      app->renderer->camera.position =
          app->renderer->camera.position +
          glm::sign(mouse_cursor) *
              glm::normalize(app->renderer->camera.target -
                             app->renderer->camera.position);
      app->prev_mouse_cursor = mouse_cursor;
    }
    if (ImGui::IsMouseDragging(2)) {
      ImVec2 mouse_pos = ImGui::GetMousePos();
      if (!app->isFirstFameDragging) {
        ImVec2 drag_delta = ImVec2(mouse_pos.x - app->prev_mouse_position.x,
                                   mouse_pos.y - app->prev_mouse_position.y);
        if (drag_delta.x != 0.0f or drag_delta.y != 0.0f) {
          if (ImGui::IsKeyPressed(ImGuiKey_LeftShift)) {

            glm::vec2 scaled_drag_delta = glm::vec2(drag_delta.x, drag_delta.y);
            glm::vec3 forward = glm::normalize(app->renderer->camera.target -
                                               app->renderer->camera.position);
            glm::vec3 W = -forward;
            glm::vec3 U =
                glm::normalize(glm::cross(app->renderer->camera.up, W));
            glm::vec3 V = glm::normalize(glm::cross(W, U));
            float movement_factor =
                2 *
                SDL_tanf((app->renderer->camera.aspectRatio * SDL_PI_F) /
                         180.0f) *
                glm::length(app->renderer->camera.position) *
                (1 / render_width) * 100.0f;
            app->renderer->camera.position =
                app->renderer->camera.position -
                movement_factor * U * scaled_drag_delta.x +
                movement_factor * V * scaled_drag_delta.y;
            app->renderer->camera.target =
                app->renderer->camera.target -
                movement_factor * U * scaled_drag_delta.x +
                movement_factor * V * scaled_drag_delta.y;
            // LOG_INFO("prev mouse %f %f", app->prev_mouse_position.x,
            //          app->prev_mouse_position.y);
            // LOG_INFO("target %f %f %f", app->renderer->camera.target.x,
            //          app->renderer->camera.target.y,
            //          app->renderer->camera.target.z);
            // LOG_INFO("position %f %f %f", app->renderer->camera.position.x,
            //          app->renderer->camera.position.y,
            //          app->renderer->camera.position.z);
            // LOG_INFO("drag delta %f %f", drag_delta.x, drag_delta.y);
            // LOG_INFO("movement_factor %f", movement_factor);

          } else {
            glm::vec2 scaled_drag_delta =
                normalize(glm::vec2(drag_delta.x, drag_delta.y));
            glm::vec3 forward = glm::normalize(app->renderer->camera.target -
                                               app->renderer->camera.position);
            glm::vec3 W = -forward;
            glm::vec3 U =
                glm::normalize(glm::cross(app->renderer->camera.up, W));
            glm::vec3 V = glm::normalize(glm::cross(W, U));
            float movement_factor = 0.045f;
            app->renderer->camera.target =
                app->renderer->camera.target -
                movement_factor * U * scaled_drag_delta.x +
                movement_factor * V * scaled_drag_delta.y;
          } //! fix pan for close to zero and fix gimbal lock in rotation
        }
      }
      app->isFirstFameDragging = false;
      app->prev_mouse_position = glm::vec2(mouse_pos.x, mouse_pos.y);

      // ImVec2 drag_delta = ImGui::GetMouseDragDelta(2);
      // glm::vec2 norm_drag_delta =
      //     glm::normalize(glm::vec2(drag_delta.x, drag_delta.y));
      // glm::vec3 forward = glm::normalize(app->renderer->camera.target -
      //                                    app->renderer->camera.position);
      // glm::vec3 W = -forward;
      // glm::vec3 U = glm::normalize(glm::cross(app->renderer->camera.up, W));
      // glm::vec3 V = glm::normalize(glm::cross(W, U));
      // glm::vec3 drag_direction = norm_drag_delta.x * U + norm_drag_delta.y *
      // V; glm::vec3 axis = glm::normalize(glm::cross(drag_direction, W));
      // glm::quat rot_quat = glm::angleAxis((1.0f * SDL_PI_F) / 180.0f, axis);
      // glm::mat4 rot_mat = glm::mat4_cast(rot_quat);
      // app->renderer->camera.
    } else {
      app->isFirstFameDragging = true;
    }
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
