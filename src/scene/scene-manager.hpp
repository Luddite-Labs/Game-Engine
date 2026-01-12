#pragma once

#include <scene/scene.hpp>

class SceneManager {
private:
  inline static SceneManager *singleton = nullptr;

  SceneManager() {}

public:
  std::vector<Scene> scenes;
  uint32_t active_scene_index; //! add update logic
  static void init() { singleton = new SceneManager(); }
  static void destroy() { delete singleton; }
  static SceneManager *getSingleton() {
#ifdef GAME_ENGINE_DEBUG_MODE
    SDL_assert(singleton != nullptr);
#endif
    return singleton;
  }
};