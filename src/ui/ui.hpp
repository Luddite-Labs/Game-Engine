#pragma once
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <cstdint>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>
#include <renderer/renderer.hpp>

class UI {
private:
  uint64_t m_mem_arena_size;
  char *m_mem_arena;
  inline static UI *singleton = nullptr;

  UI(SDL_Window *window);
  ~UI();

public:
  static void init(SDL_Window *window) { singleton = new UI(window); }
  static void destroy() { delete singleton; }
  static UI *getSingleton() {
#ifdef GAME_ENGINE_DEBUG_MODE
    SDL_assert(singleton != nullptr);
#endif
    return singleton;
  }
  SDL_AppResult processEvent(SDL_Event *event);
  void beginFrame();
  void endFrame(SDL_Window* window);
};