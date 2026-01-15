#include <string>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <misc/log.hpp>
#include <misc/slot-map.hpp>
#include <renderer/types.hpp>

using ShaderStorageType = SlotMap<std::vector<RE::Shader::Data>, RE::Shader::Data, RE::Shader::Handle>;

// Shader Storage
namespace ShS {
void init(SDL_GPUDevice *device);
void destroy();

RE::Shader::Handle
createShader(const std::string &shader_file,
		const std::vector<RE::Shader::Definition> &defines);
void refShader(RE::Shader::Handle shader);
void destroyShader(RE::Shader::Handle shader);
RE::Shader::Type getShaderType(RE::Shader::Handle shader);
// const char *getShaderFilePath(RE::Shader::Handle shader);
uint32_t getShaderNumSamplers(RE::Shader::Handle shader);
uint32_t getShaderNumStorageTextures(RE::Shader::Handle shader);
uint32_t getShaderNumStorageBuffers(RE::Shader::Handle shader);
uint32_t getShaderNumUniformBuffers(RE::Shader::Handle shader);
SDL_GPUShader* getShaderGPUHandle(RE::Shader::Handle shader);
bool isValid(RE::Shader::Handle camera);
}; // namespace ShS