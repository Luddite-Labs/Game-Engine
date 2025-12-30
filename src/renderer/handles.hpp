#pragma once

#include "misc/utils.hpp"

namespace handle {
#ifndef GAME_ENGINE_DEBUG_MODE
struct Camera {
  Handle handle;
};
#else
typedef Handle Camera;
#endif

#ifndef GAME_ENGINE_DEBUG_MODE
struct Sampler {
  Handle handle;
};
#else
typedef Handle Sampler;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Texture {
  Handle handle;
};
#else
typedef Handle Texture;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Mesh {
  Handle handle;
};
#else
typedef Handle Mesh;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Material {
  Handle handle;
};
#else
typedef Handle Material;
#endif
#ifndef GAME_ENGINE_DEBUG_MODE
struct Shader {
  Handle handle;
};
#else
typedef Handle Shader;
#endif
} // namespace handle
