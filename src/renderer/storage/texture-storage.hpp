#pragma once

#include <SDL3/SDL_assert.h>

#include <misc/slot-map.hpp>
#include <renderer/types.hpp>

using TextureStorageType = SlotMap<std::vector<data::Texture>, data::Texture>;

namespace TS {
// Texture
void init(SDL_GPUDevice *device);
void destroy();
handle::Texture createTexture(uint32_t width, uint32_t height,
		// TextureType type, ! maybe other texture supports in the future
		TextureUsageFlags usage_flags,
		TextureFormat format);
void refTexture(handle::Texture texture);
void destroyTexture(handle::Texture texture);
uint32_t getTextureWidth(handle::Texture texture);
uint32_t getTextureHeight(handle::Texture texture);
TextureFormat getTextureFormat(handle::Texture texture);
uint32_t getTextureUsageFlags(handle::Texture texture);
handle::Sampler getTextureSampler(handle::Texture texture);
void setTextureWidth(handle::Texture texture, uint32_t width);
void setTextureHeight(handle::Texture texture, uint32_t height);
void setTextureFormat(handle::Texture texture, TextureFormat format);
void setTextureUsageFlags(handle::Texture texture, uint32_t usage_flags);
void setTextureSampler(handle::Texture texture, handle::Sampler sampler);
void uploadBufferToTexture(handle::Texture texture,
		std::shared_ptr<uint8_t> buffer, size_t offset,
		size_t count);
SDL_GPUTexture *getTextureGPUHandle(handle::Texture texture);
}; // namespace TS