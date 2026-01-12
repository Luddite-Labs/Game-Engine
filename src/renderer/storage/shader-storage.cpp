#pragma once

#include <renderer/storage/shader-storage.hpp>

namespace {

SDL_GPUDevice *m_GPU_device;
ShaderStorageType shader_storage;
}; // namespace

// Shader Storage
namespace ShS {
void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	SDL_assert(SDL_ShaderCross_Init());
}
void destroy() {
	SDL_ShaderCross_Quit();
}

handle::Shader
createShader(const std::string &shader_file,
		const std::vector<data::ShaderDefinition> &defines) {
	//! migrate to specialization constants
	//! add caching based on defines
	SDL_assert(m_GPU_device != nullptr);

	ShaderType shader_type;
	if (shader_file.find(".vert") != std::string::npos) {
		shader_type = ShaderType::VERTEX;
	} else if (shader_file.find(".frag") != std::string::npos) {
		shader_type = ShaderType::FRAGMENT;
	} else {
		SDL_assert(false); // Unsupported stage
	}

	std::vector<SDL_ShaderCross_HLSL_Define> shadercross_defines;
	for (const auto &define : defines) {
		shadercross_defines.push_back({
				.name = const_cast<char *>(define.name),
				.value = const_cast<char *>(define.value),
		});
	}
	shadercross_defines.push_back({ .name = nullptr,
			.value = nullptr });

	size_t data_size;
	uint8_t *buffer = static_cast<uint8_t *>(
			SDL_LoadFile(shader_file.c_str(), &data_size));
	SDL_ShaderCross_HLSL_Info hlsl_info = {
		.source = reinterpret_cast<char *>(buffer),
		.entrypoint = "main",
		.include_dir = nullptr, //! add option for includes later
		.defines =
				shadercross_defines.data(),
		.shader_stage = static_cast<SDL_ShaderCross_ShaderStage>(shader_type)
	};

	data::Shader shader_data = {};
	auto spirv_ir = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &data_size);
	CHECK_AND_PRINT_SDL_ERROR();
	SDL_ShaderCross_GraphicsShaderMetadata *shader_metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(static_cast<uint8_t *>(spirv_ir), data_size, 0);
	CHECK_AND_PRINT_SDL_ERROR();
	SDL_ShaderCross_SPIRV_Info spirv_info = { .bytecode = static_cast<uint8_t *>(spirv_ir), .bytecode_size = data_size, .entrypoint = "main", .shader_stage = static_cast<SDL_ShaderCross_ShaderStage>(shader_type) };
	shader_data.gpu_handle = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(m_GPU_device, &spirv_info, &shader_metadata->resource_info, 0);
	CHECK_AND_PRINT_SDL_ERROR();
	SDL_free(spirv_ir);
	SDL_assert(shader_data.gpu_handle != nullptr);
	return shader_storage.insert(shader_data);
}
void refShader(handle::Shader shader) {
	shader_storage.ref(shader);
}
void destroyShader(handle::Shader shader) {
	shader_storage.erase(shader);
}
ShaderType getShaderType(handle::Shader shader) {
	return shader_storage.get(shader).type;
}
// const char *getShaderFilePath(handle::Shader shader);
uint32_t getShaderNumSamplers(handle::Shader shader) {
	return shader_storage.get(shader).num_samplers;
}
uint32_t getShaderNumStorageTextures(handle::Shader shader) {
	return shader_storage.get(shader).num_storage_textures;
}
uint32_t getShaderNumStorageBuffers(handle::Shader shader) {
	return shader_storage.get(shader).num_storage_buffers;
}
uint32_t getShaderNumUniformBuffers(handle::Shader shader) {
	return shader_storage.get(shader).num_uniform_buffers;
}
SDL_GPUShader *getShaderGPUHandle(handle::Shader shader) {
	return shader_storage.get(shader).gpu_handle;
}
}; // namespace ShS
