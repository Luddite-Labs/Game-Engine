#include <renderer/graph.hpp>

namespace {
SDL_GPUDevice *GPU_device;
uint32_t current_free_handle_index;
std::unordered_map<std::string, RE::Graph::Handle> handle_name_map;
std::unordered_map<std::string, uint32_t> pass_name_map;
std::vector<RE::Graph::Pass> passes;
std::vector<void *> gpu_handles;
std::vector<std::function<void(uint32_t, uint32_t)>> creation_callbacks;
std::vector<std::function<void(uint32_t)>> destruction_callbacks;
std::vector<uint32_t> handle_usage_flags;
std::vector<std::vector<RE::Graph::Pass *>> sorted_passes;
}; // namespace

namespace RE::Graph {
void init(SDL_GPUDevice *m_GPU_device) {
	GPU_device = m_GPU_device;
	current_free_handle_index = 0;
}

void destroy() {
}

void clear() {
	current_free_handle_index = 0;
	handle_name_map.clear();
	pass_name_map.clear();
	passes.clear();
	gpu_handles.clear();
	creation_callbacks.clear();
	handle_usage_flags.clear();
	sorted_passes.clear();
}
void addPass(std::string pass_name, PassType type, std::function<void(SDL_GPUCommandBuffer *command_buffer)> execute) {
	pass_name_map[pass_name] = passes.size();
	passes.push_back({ .name = pass_name,
			.type = type,
			.execute = execute });
}

void importTexture(std::string pass_name, std::string texture_name, RE::Texture::Handle handle) {
	if (handle_name_map.find(texture_name) == handle_name_map.end()) {
		handle_name_map[texture_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(TS::getTextureGPUHandle(handle));
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([](uint32_t usage, uint32_t slot) {
		});
		destruction_callbacks.push_back([](uint32_t slot) {
		});
		current_free_handle_index += 1;
	}
}

// Usage does not need to be given
void createTexture(std::string pass_name, std::string texture_name, SDL_GPUTextureCreateInfo texture_create_desc) {
	if (handle_name_map.find(texture_name) == handle_name_map.end()) {
		handle_name_map[texture_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(nullptr);
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([texture_create_desc](SDL_GPUTextureUsageFlags usage, uint32_t slot) {
			auto create_info = texture_create_desc;
			create_info.usage = usage;
			gpu_handles[slot] = SDL_CreateGPUTexture(GPU_device, &create_info);
		});
		destruction_callbacks.push_back([&](uint32_t slot) {
			SDL_ReleaseGPUTexture(GPU_device, reinterpret_cast<SDL_GPUTexture *>(gpu_handles[slot]));
		});
		current_free_handle_index += 1;
	}
}

void readTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	pass.inputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
			break;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
			break;
		case PassType::COPY:
			break;
	}
}

void writeTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
			break;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE;
			break;
		case PassType::COPY:
			break;
	}
}

void writeTextureDepth(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
			break;
		case PassType::COMPUTE:
			break;
		case PassType::COPY:
			break;
	}
}

void readWriteTexture(std::string pass_name, std::string texture_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[texture_name];
	pass.inputs.push_back(handle);
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
			break;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE;
			break;
		case PassType::COPY:
			break;
	}
}

void importBuffer(std::string pass_name, std::string buffer_name, RE::Buffer::Handle handle) {
	if (handle_name_map.find(buffer_name) == handle_name_map.end()) {
		handle_name_map[buffer_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(BS::getBufferGPUHandle(handle));
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([](uint32_t usage, uint32_t slot) {
		});
		destruction_callbacks.push_back([](uint32_t slot) {
		});
		current_free_handle_index += 1;
	}
}

// Usage does not need to be given
void createBuffer(std::string pass_name, std::string buffer_name, SDL_GPUBufferCreateInfo buffer_create_desc) {
	if (handle_name_map.find(buffer_name) == handle_name_map.end()) {
		handle_name_map[buffer_name] = { .slot = current_free_handle_index, .version = 1 };
		gpu_handles.push_back(nullptr);
		handle_usage_flags.push_back(0);
		creation_callbacks.push_back([buffer_create_desc](SDL_GPUBufferUsageFlags usage, uint32_t slot) {
			auto create_info = buffer_create_desc;
			create_info.usage = usage;
			gpu_handles[slot] = SDL_CreateGPUBuffer(GPU_device, &create_info);
		});
		destruction_callbacks.push_back([&](uint32_t slot) {
			SDL_ReleaseGPUBuffer(GPU_device, reinterpret_cast<SDL_GPUBuffer *>(gpu_handles[slot]));
		});
		current_free_handle_index += 1;
	}
}

void readIndexBuffer(std::string pass_name, std::string buffer_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[buffer_name];
	pass.inputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_BUFFERUSAGE_INDEX;
			break;
		case PassType::COMPUTE:
			break;
		case PassType::COPY:
			break;
	}
}

void readBuffer(std::string pass_name, std::string buffer_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[buffer_name];
	pass.inputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			handle_usage_flags[handle.slot] |= SDL_GPU_BUFFERUSAGE_VERTEX;
			break;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
			break;
		case PassType::COPY:
			break;
	}
}

void writeBuffer(std::string pass_name, std::string buffer_name) {
	Pass &pass = passes[pass_name_map[pass_name]];
	Handle &handle = handle_name_map[buffer_name];
	handle.version += 1;
	pass.outputs.push_back(handle);
	switch (pass.type) {
		case PassType::RENDER:
			break;
		case PassType::COMPUTE:
			handle_usage_flags[handle.slot] |= SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
			break;
		case PassType::COPY:
			break;
	}
}

bool handleNotDependent(Handle &handle) {
	int r = sorted_passes.size();
	for (int i = 0; i < r; i++) {
		for (int j = 0; j < sorted_passes[i].size(); j++) {
			for (auto &output : sorted_passes[i][j]->outputs) {
				if (output.slot == handle.slot && output.version == handle.version) {
					return true;
				}
			}
		}
	}
	return false;
}

void *getDeviceHandle(std::string handle_name) {
	return gpu_handles[handle_name_map[handle_name].slot];
}

void compile() {
	std::vector<RE::Graph::Pass *> q;
	std::vector<RE::Graph::Pass *> q_r;
	for (auto &pass : passes) {
		q.push_back(&pass);
	}
	int index = 0;
	while (!q.empty()) {
		sorted_passes.emplace_back();
		for (int i = 0; i < q.size(); i++) {
			bool no_dependencies = true;
			for (auto &handle : q[i]->inputs) {
				no_dependencies &= (handle.version == 1 or handleNotDependent(handle));
			}
			if (no_dependencies) {
				sorted_passes[index].push_back(q[i]);
			} else {
				q_r.push_back(q[i]);
			}
		}
		q = q_r;
		q_r.clear();
		index += 1;
	}
}

void execute() {
	compile();

	for (int i = 0; i < creation_callbacks.size(); i++) {
		creation_callbacks[i](handle_usage_flags[i], i);
	}

	for (auto &stage : sorted_passes) {
		SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(GPU_device);
		for (auto &pass : stage) {
			pass->execute(command_buffer);
		}
		SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);
		SDL_WaitForGPUFences(GPU_device, true, &fence, 1);
		SDL_ReleaseGPUFence(GPU_device, fence);
	}

	for (int i = 0; i < destruction_callbacks.size(); i++) {
		destruction_callbacks[i](i);
	}

	clear();
}
}; // namespace RE::Graph