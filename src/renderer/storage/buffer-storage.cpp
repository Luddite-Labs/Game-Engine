#include <renderer/storage/buffer-storage.hpp>

namespace {
SDL_GPUDevice *m_GPU_device;
BufferStorageType buffer_storage;
} // namespace

namespace BS {
// Camera
void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
}
void destroy() {}

RE::Buffer::Handle createBuffer(RE::Buffer::Usage usage, uint32_t size) {
	SDL_GPUBufferCreateInfo create_info = { .usage = static_cast<SDL_GPUBufferUsageFlags>(usage), .size = size };
	SDL_GPUBuffer *gpu_handle = SDL_CreateGPUBuffer(m_GPU_device, &create_info);
	RE::Buffer::Handle buffer = buffer_storage.insert({ .usage = usage, .size = size, .gpu_handle = gpu_handle });
	return buffer;
}
void refBuffer(RE::Buffer::Handle buffer) {
	buffer_storage.ref(buffer);
}
void destroyBuffer(RE::Buffer::Handle buffer) {
	buffer_storage.erase(buffer);
}
uint32_t getBufferSize(RE::Buffer::Handle buffer) {
	return buffer_storage.get(buffer).size;
}
RE::Buffer::Usage getBufferUsage(RE::Buffer::Handle buffer) {
	buffer_storage.get(buffer).usage;
}
SDL_GPUBuffer *getBufferGPUHandle(RE::Buffer::Handle buffer) {
	return buffer_storage.get(buffer).gpu_handle;
}
bool isValid(RE::Buffer::Handle buffer) {
	return buffer_storage.isValid(buffer);
}
}; // namespace BS