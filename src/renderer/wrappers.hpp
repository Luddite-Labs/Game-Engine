#pragma once

#include "SDL3/SDL_gpu.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "misc/slot-map.hpp"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <renderer/renderer.hpp>
#include <variant>

#define SHARED()                                 \
	struct Shared {                              \
		Handle handle;                           \
                                                 \
	public:                                      \
		Shared(Handle handle) : handle(handle) { \
		}                                        \
		Shared(const Shared &other) {            \
			destroy(handle);                     \
			ref(other.handle);                   \
			handle = other.handle;               \
		}                                        \
		~Shared() {                              \
			destroy(handle);                     \
		}                                        \
		void operator=(const Shared &other) {    \
			destroy(handle);                     \
			ref(other.handle);                   \
			handle = other.handle;               \
		}                                        \
		void reset() {                           \
			destroy(handle);                     \
			handle = { 0, 0 };                   \
		}                                        \
		bool valid() {                           \
			return isValid(handle);              \
		}                                        \
	};

namespace RE {

namespace Camera {
SHARED();
}; // namespace Camera

namespace Sampler {
SHARED();
}; // namespace Sampler

namespace Texture {
SHARED();
}; // namespace Texture
namespace Mesh {
SHARED();
}; // namespace Mesh
namespace Material {
SHARED();
}; // namespace Material
namespace Shader {
SHARED();
}; // namespace Shader
}; // namespace RE
