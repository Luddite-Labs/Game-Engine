#pragma once

#include <renderer/storage/sampler-storage.hpp>
#include <renderer/storage/texture-storage.hpp>

namespace {
SDL_GPUDevice *m_GPU_device;
TextureStorageType texture_storage;
}; // namespace

namespace TS {
// Texture
void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
}
void destroy() {}
handle::Texture createTexture(uint32_t width, uint32_t height,
		// TextureType type, ! maybe other texture supports in the future
		TextureUsageFlags usage_flags,
		TextureFormat format) {
	SDL_assert(m_GPU_device);
	const SDL_GPUTextureCreateInfo tex_info{
		.type = static_cast<SDL_GPUTextureType>(TextureType::TEXTURE_2D),
		.format = static_cast<SDL_GPUTextureFormat>(format),
		.usage = static_cast<SDL_GPUTextureUsageFlags>(usage_flags),
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.props = 0
	};
	SDL_GPUTexture *gpu_handle = SDL_CreateGPUTexture(m_GPU_device, &tex_info);
	return texture_storage.insert({
			.gpu_handle = gpu_handle,
			.width = width,
			.height = height,
			.format = format,
	});
}
void refTexture(handle::Texture texture) {
	texture_storage.ref(texture);
}
void destroyTexture(handle::Texture texture) {
	texture_storage.erase(texture);
}
uint32_t getTextureWidth(handle::Texture texture) {
	return texture_storage.get(texture).width;
}
uint32_t getTextureHeight(handle::Texture texture) {
	return texture_storage.get(texture).height;
}
TextureFormat getTextureFormat(handle::Texture texture) {
	return texture_storage.get(texture).format;
}
uint32_t getTextureUsageFlags(handle::Texture texture) {
	return texture_storage.get(texture).usage_flags;
}
handle::Sampler getTextureSampler(handle::Texture texture) {
	return texture_storage.get(texture).sampler;
}
void setTextureWidth(handle::Texture texture, uint32_t width) {
	texture_storage.get(texture).width = width;
	texture_storage.setIsEdited(texture);
}
void setTextureHeight(handle::Texture texture, uint32_t height) {
	texture_storage.get(texture).height = height;
	texture_storage.setIsEdited(texture);
}
void setTextureFormat(handle::Texture texture, TextureFormat format) {
	texture_storage.get(texture).format = format;
	texture_storage.setIsEdited(texture);
}
void setTextureUsageFlags(handle::Texture texture, uint32_t usage_flags) {
	texture_storage.get(texture).usage_flags = usage_flags;
	texture_storage.setIsEdited(texture);
}
void setTextureSampler(handle::Texture texture, handle::Sampler sampler) {
	texture_storage.get(texture).sampler = sampler;
	SaS::refSampler(sampler);
	// texture_storage.setIsEdited(texture); //! no change texture state so no
	// refresh needed?
}
void uploadBufferToTexture(handle::Texture texture,
		std::shared_ptr<uint8_t> buffer, size_t offset,
		size_t count) {
	auto &tex = texture_storage.get(texture);
	SDL_GPUTransferBufferCreateInfo transfer_create_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = static_cast<uint32_t>(count)
	};
	SDL_GPUTransferBuffer *transfer_buffer =
			SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);
	uint8_t *transfer_buffer_ptr = static_cast<uint8_t *>(
			SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer, false));

	SDL_memcpy(transfer_buffer_ptr, buffer.get(), count);

	SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer);
	SDL_GPUCommandBuffer *copy_cmd_buffer =
			SDL_AcquireGPUCommandBuffer(m_GPU_device);
	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buffer);
	SDL_GPUTextureTransferInfo tex_tranfer_location = {
		.transfer_buffer = transfer_buffer, .offset = 0
	};
	SDL_GPUTextureRegion tex_transfer_region = {
		.texture = tex.gpu_handle, .w = tex.width, .h = tex.height, .d = 1
	};
	SDL_UploadToGPUTexture(copy_pass, &tex_tranfer_location,
			&tex_transfer_region, false);
	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(copy_cmd_buffer);
	SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
}
SDL_GPUTexture *getTextureGPUHandle(handle::Texture texture) {
	SDL_assert(TextureStorageType::isValid(texture));
	return texture_storage.get(texture).gpu_handle;
}
}; // namespace TS