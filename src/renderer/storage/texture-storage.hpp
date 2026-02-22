#pragma once

#include <SDL3/SDL_assert.h>

#include <misc/slot-map.hpp>
#include <renderer/types.hpp>

using TextureStorageType = SlotMap<std::vector<RE::Texture::Data>, RE::Texture::Data, RE::Texture::Handle>;

namespace TS {
// Texture
void init(SDL_GPUDevice *device);
void destroy();
RE::Texture::Handle createTexture(uint32_t width, uint32_t height,
		// TextureType type, ! maybe other texture supports in the future
		RE::Texture::UsageFlags usage_flags,
		RE::Texture::Format format, RE::Texture::SampleCount sample_count, bool generate_mip_maps);
void refTexture(RE::Texture::Handle texture);
void destroyTexture(RE::Texture::Handle texture);
uint32_t getTextureWidth(RE::Texture::Handle texture);
uint32_t getTextureHeight(RE::Texture::Handle texture);
RE::Texture::Format getTextureFormat(RE::Texture::Handle texture);
uint32_t getTextureUsageFlags(RE::Texture::Handle texture);
RE::Sampler::Handle getTextureSampler(RE::Texture::Handle texture);
void setTextureWidth(RE::Texture::Handle texture, uint32_t width);
void setTextureHeight(RE::Texture::Handle texture, uint32_t height);
void setTextureFormat(RE::Texture::Handle texture, RE::Texture::Format format);
void setTextureUsageFlags(RE::Texture::Handle texture, uint32_t usage_flags);
void setTextureSampler(RE::Texture::Handle texture, RE::Sampler::Handle sampler);
void uploadBufferToTexture(RE::Texture::Handle texture,
		std::shared_ptr<uint8_t> buffer, size_t offset,
		size_t count);
SDL_GPUTexture *getTextureGPUHandle(RE::Texture::Handle texture);
bool isValid(RE::Texture::Handle texture);
}; // namespace TS