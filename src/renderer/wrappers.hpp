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

#define SHARED()                                          \
	struct Shared {                                       \
		Handle handle;                                    \
                                                          \
	public:                                               \
		Shared() : handle({ 0, 0 }) {                     \
		}                                                 \
		explicit Shared(Handle handle) : handle(handle) { \
		}                                                 \
		Shared(const Shared &other) {                     \
			/* ref first in case same handle */           \
			if (other.handle != Handle{ 0, 0 })           \
				ref(other.handle);                        \
			handle = other.handle;                        \
		}                                                 \
		~Shared() {                                       \
			if (handle != Handle{ 0, 0 })                 \
				destroy(handle);                          \
		}                                                 \
		void operator=(const Shared &other) {             \
			/* ref first in case same handle */           \
			if (other.handle != Handle{ 0, 0 })           \
				ref(other.handle);                        \
			if (handle != Handle{ 0, 0 })                 \
				destroy(handle);                          \
			handle = other.handle;                        \
		}                                                 \
		void reset() {                                    \
			if (handle != Handle{ 0, 0 })                 \
				destroy(handle);                          \
			handle = { 0, 0 };                            \
		}                                                 \
		bool valid() {                                    \
			return isValid(handle);                       \
		}                                                 \
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
namespace Light {
SHARED();
}; // namespace Light
}; // namespace RE


