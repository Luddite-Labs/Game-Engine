#pragma once

#include <string>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <misc/log.hpp>
#include <misc/slot-map.hpp>
#include <renderer/types.hpp>

using ShaderStorageType = SlotMap<std::vector<data::Shader>, data::Shader>;

// Shader Storage
namespace ShS {
void init(SDL_GPUDevice *device);
void destroy();

handle::Shader
createShader(const std::string &shader_file,
		const std::vector<data::ShaderDefinition> &defines);
void refShader(handle::Shader shader);
void destroyShader(handle::Shader shader);
ShaderType getShaderType(handle::Shader shader);
// const char *getShaderFilePath(handle::Shader shader);
uint32_t getShaderNumSamplers(handle::Shader shader);
uint32_t getShaderNumStorageTextures(handle::Shader shader);
uint32_t getShaderNumStorageBuffers(handle::Shader shader);
uint32_t getShaderNumUniformBuffers(handle::Shader shader);
SDL_GPUShader* getShaderGPUHandle(handle::Shader shader);
}; // namespace ShS