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
RE::Texture::Handle createTexture(uint32_t width, uint32_t height,
		// TextureType type, ! maybe other texture supports in the future
		RE::Texture::UsageFlags usage_flags,
		RE::Texture::Format format, RE::Texture::SampleCount sample_count, bool generate_mip_maps) {
	SDL_assert(m_GPU_device);
	uint32_t mip_levels = 1;
	if (generate_mip_maps) {
		mip_levels = std::floor(std::log2(std::max(width, height))) + 1;
	}
	const SDL_GPUTextureCreateInfo tex_info{
		.type = static_cast<SDL_GPUTextureType>(RE::Texture::Type::TEXTURE_2D),
		.format = static_cast<SDL_GPUTextureFormat>(format),
		.usage = static_cast<SDL_GPUTextureUsageFlags>(usage_flags),
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = mip_levels,
		.sample_count = static_cast<SDL_GPUSampleCount>(sample_count),
		.props = 0
	};
	SDL_GPUTexture *gpu_handle = SDL_CreateGPUTexture(m_GPU_device, &tex_info);
	return texture_storage.insert({ .gpu_handle = gpu_handle,
			.width = width,
			.height = height,
			.format = format,
			.mip_levels = mip_levels });
}
void refTexture(RE::Texture::Handle texture) {
	texture_storage.ref(texture);
}
void destroyTexture(RE::Texture::Handle texture) {
	texture_storage.erase(texture);
}
uint32_t getTextureWidth(RE::Texture::Handle texture) {
	return texture_storage.get(texture).width;
}
uint32_t getTextureHeight(RE::Texture::Handle texture) {
	return texture_storage.get(texture).height;
}
RE::Texture::Format getTextureFormat(RE::Texture::Handle texture) {
	return texture_storage.get(texture).format;
}
uint32_t getTextureUsageFlags(RE::Texture::Handle texture) {
	return texture_storage.get(texture).usage_flags;
}
RE::Sampler::Handle getTextureSampler(RE::Texture::Handle texture) {
	return texture_storage.get(texture).sampler;
}
void setTextureWidth(RE::Texture::Handle texture, uint32_t width) {
	texture_storage.get(texture).width = width;
	texture_storage.setIsEdited(texture);
}
void setTextureHeight(RE::Texture::Handle texture, uint32_t height) {
	texture_storage.get(texture).height = height;
	texture_storage.setIsEdited(texture);
}
void setTextureFormat(RE::Texture::Handle texture, RE::Texture::Format format) {
	texture_storage.get(texture).format = format;
	texture_storage.setIsEdited(texture);
}
void setTextureUsageFlags(RE::Texture::Handle texture, uint32_t usage_flags) {
	texture_storage.get(texture).usage_flags = usage_flags;
	texture_storage.setIsEdited(texture);
}
void setTextureSampler(RE::Texture::Handle texture, RE::Sampler::Handle sampler) {
	texture_storage.get(texture).sampler = sampler;
	SaS::refSampler(sampler);
	// texture_storage.setIsEdited(texture); //! no change texture state so no
	// refresh needed?
}
void uploadBufferToTexture(RE::Texture::Handle texture,
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
	if (tex.mip_levels > 1) {
		SDL_GenerateMipmapsForGPUTexture(copy_cmd_buffer, tex.gpu_handle);
	}
	SDL_SubmitGPUCommandBuffer(copy_cmd_buffer);
	SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer);
}
SDL_GPUTexture *getTextureGPUHandle(RE::Texture::Handle texture) {
	SDL_assert(texture_storage.isValid(texture));
	return texture_storage.get(texture).gpu_handle;
}
bool isValid(RE::Texture::Handle texture) {
	return texture_storage.isValid(texture);
}
}; // namespace TS