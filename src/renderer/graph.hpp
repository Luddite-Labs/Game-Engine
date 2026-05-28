#pragma once

#include <functional>
#include <queue>
#include <renderer/storage/texture-storage.hpp>
#include <string>
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
}; // namespace RE::Graph
namespace {
static SDL_GPUDevice *GPU_device;
static uint32_t current_free_handle_index;
static std::unordered_map<std::string, RE::Graph::Handle> handle_name_map;
static std::unordered_map<std::string, uint32_t> pass_name_map;
static std::vector<RE::Graph::Pass> passes;
static std::vector<void *> gpu_handles;
static std::vector<std::function<void(uint32_t)>> creation_callbacks;
static std::vector<std::function<void()>> destruction_callbacks;
static std::vector<uint32_t> handle_usage_flags;
static std::vector<std::vector<RE::Graph::Pass *>> sorted_passes;
}; // namespace

namespace RE::Graph {
inline void init(SDL_GPUDevice *m_GPU_device) {
	GPU_device = m_GPU_device;
	uint32_t current_free_handle_index = 0;
}

inline void destroy() {
}

inline void clear() {
	current_free_handle_index = 0;
	handle_name_map.clear();
	pass_name_map.clear();
	passes.clear();
	gpu_handles.clear();
	creation_callbacks.clear();
	handle_usage_flags.clear();
	sorted_passes.clear();
}
inline void addPass(std::string pass_name, PassType type, std::function<void(SDL_GPUCommandBuffer *command_buffer)> execute) {
	pass_name_map[pass_name] = passes.size();
	passes.push_back({ .name = pass_name,
			.type = type,
			.execute = execute });
}
inline void importTexture(std::string pass_name, std::string texture_name, RE::Texture::Handle handle) {
	if (handle_name_map.find(texture_name) == handle_name_map.end()) {
		handle_name_map[texture_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(TS::getTextureGPUHandle(handle));
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([&](uint32_t usage) {
		});
		destruction_callbacks.push_back([&]() {
		});
		current_free_handle_index += 1;
	}
}

// Usage does not need to be given
inline void createTexture(std::string pass_name, std::string texture_name, SDL_GPUTextureCreateInfo texture_create_desc) {
	if (handle_name_map.find(texture_name) == handle_name_map.end()) {
		handle_name_map[texture_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(nullptr);
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([&](SDL_GPUTextureUsageFlags usage) {
			texture_create_desc.usage = usage;
			gpu_handles[current_free_handle_index] = SDL_CreateGPUTexture(GPU_device, &texture_create_desc);
		});
		destruction_callbacks.push_back([&]() {
			SDL_ReleaseGPUTexture(GPU_device, reinterpret_cast<SDL_GPUTexture *>(gpu_handles[current_free_handle_index]));
		});
		current_free_handle_index += 1;
	}
}

inline void readTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	pass.inputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
		case PassType::COPY:
			break;
	}
}

inline void writeTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
		case PassType::COPY:
			break;
	}
}

inline void writeTextureDepth(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
		case PassType::COMPUTE:
			break;
		case PassType::COPY:
			break;
	}
}

inline void readWriteTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	pass.inputs.push_back(handle);
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE;
		case PassType::COPY:
			break;
	}
}

inline bool handleNotDependent(Handle &handle) {
	int r = sorted_passes.size();
	for (int i = 0; i < r; i++) {
		for (int j = 0; j < sorted_passes[r].size(); i++) {
			for (auto &output : sorted_passes[i][j]->outputs) {
				if (output.slot == handle.slot && output.version == handle.version) {
					return true;
				}
			}
		}
	}
	return false;
}

inline void *getDeviceHandle(Handle &handle) {
	return gpu_handles[handle.slot];
}

inline void compile() {
	std::vector<RE::Graph::Pass *> q;
	for (auto &pass : passes) {
		q.push_back(&pass);
	}
	int index = 0;
	while (!q.empty()) {
		sorted_passes.emplace_back();
		for (int i = 0; i < q.size(); i++) {
			bool no_dependencies = true;
			for (auto &handle : q[i]->inputs) {
				no_dependencies |= (handle.version == 1 or handleNotDependent(handle));
			}
			if (no_dependencies) {
				q.erase(q.begin() + i);
				sorted_passes[index].push_back(q[i]);
			}
		}
		index += 1;
	}
}

inline void execute() {
	compile();

	for (int i = 0; i < creation_callbacks.size(); i++) {
		creation_callbacks[i](handle_usage_flags[i]);
	}

	for (auto &stage : sorted_passes) {
		SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(GPU_device);
		for (auto &pass : stage) {
			pass->execute(command_buffer);
		}
		SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);
		SDL_WaitForGPUFences(GPU_device, true, &fence, 1);
	}

	for (int i = 0; i < destruction_callbacks.size(); i++) {
		destruction_callbacks[i]();
	}

    clear();
}
}; // namespace RE::Graph