#pragma once

#include <functional>
#include <queue>
#include <renderer/storage/buffer-storage.hpp>
#include <renderer/storage/texture-storage.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace RE::Graph {
struct Handle {
	uint32_t slot;
	uint32_t version;
};
enum PassType {
	COPY,
	RENDER,
	COMPUTE
};
struct Pass {
	std::string name;
	PassType type;
	std::function<void(SDL_GPUCommandBuffer *command_buffer)> execute;
	std::vector<Handle> inputs;
	std::vector<Handle> outputs;
};

void init(SDL_GPUDevice *m_GPU_device);
void destroy();
void clear();

void addPass(std::string pass_name, PassType type, std::function<void(SDL_GPUCommandBuffer *command_buffer)> execute);

void importTexture(std::string pass_name, std::string texture_name, RE::Texture::Handle handle);
void createTexture(std::string pass_name, std::string texture_name, SDL_GPUTextureCreateInfo texture_create_desc);
void readTexture(std::string pass_name, std::string texture_name);
void writeTexture(std::string pass_name, std::string texture_name);
void writeTextureDepth(std::string pass_name, std::string texture_name);
void readWriteTexture(std::string pass_name, std::string texture_name);

void importBuffer(std::string pass_name, std::string buffer_name, RE::Buffer::Handle handle);
void createBuffer(std::string pass_name, std::string buffer_name, SDL_GPUBufferCreateInfo buffer_create_desc);
void readIndexBuffer(std::string pass_name, std::string buffer_name);
void readBuffer(std::string pass_name, std::string buffer_name);
void writeBuffer(std::string pass_name, std::string buffer_name);

void *getDeviceHandle(std::string handle_name);

void execute();
}; // namespace RE::Graph