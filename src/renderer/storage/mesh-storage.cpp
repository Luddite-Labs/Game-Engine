#include <renderer/storage/material-storage.hpp>
#include <renderer/storage/mesh-storage.hpp>
#include <renderer/storage/shader-storage.hpp>
#include <renderer/storage/pipeline-storage.hpp>

namespace {
SDL_GPUDevice *m_GPU_device;
MeshStorageType mesh_storage;
uint32_t max_stride = sizeof(glm::vec3);
std::unordered_map<data::VertAttributeIndex, uint32_t> stride_map = {
	{ data::VertAttributeIndex::POSITION, sizeof(glm::vec3) },
	{ data::VertAttributeIndex::NORMAL, sizeof(glm::vec3) },
	{ data::VertAttributeIndex::TANGENT, sizeof(glm::vec3) },
	{ data::VertAttributeIndex::COLOR, sizeof(glm::vec3) },
	{ data::VertAttributeIndex::UV, sizeof(glm::vec2) },
	{ data::VertAttributeIndex::INDEX, sizeof(uint16_t) },
};
std::unordered_map<data::VertAttributeIndex, data::VertAttributes> attr_index_map = {
	{ data::VertAttributeIndex::POSITION, data::VertAttributes::POSITION },
	{ data::VertAttributeIndex::NORMAL, data::VertAttributes::NORMAL },
	{ data::VertAttributeIndex::TANGENT, data::VertAttributes::TANGENT },
	{ data::VertAttributeIndex::COLOR, data::VertAttributes::COLOR },
	{ data::VertAttributeIndex::UV, data::VertAttributes::UV },
	{ data::VertAttributeIndex::INDEX, data::VertAttributes::INDEX },
};

std::unordered_map<data::VertAttributes, const char *> vert_attr_define_map = {
	{ data::VertAttributes::POSITION, "POSITION_USED" },
	{ data::VertAttributes::NORMAL, "NORMAL_USED" },
	{ data::VertAttributes::TANGENT, "TANGENT_USED" },
	{ data::VertAttributes::COLOR, "COLOR_USED" },
	{ data::VertAttributes::UV, "UV_USED" },
	{ data::VertAttributes::INDEX, "INDEX_USED" },
};

std::unordered_map<data::MaterialOptions, const char *> frag_option_define_map = {
	{ data::MaterialOptions::COLOR_TEXTURE, "COLOR_TEXTURE_USED" },
	{ data::MaterialOptions::EMISSIVE_TEXTURE, "EMISSIVE_TEXTURE_USED" },
	{ data::MaterialOptions::NORMAL_TEXTURE, "NORMAL_TEXTURE_USED" },
	{ data::MaterialOptions::METALLIC_ROUGHNESS_TEXTURE, "METALLIC_ROUGHNESS_TEXTURE_USED" },
	{ data::MaterialOptions::OCCLUSION_TEXTURE, "OCCLUSION_TEXTURE_USED" },
	{ data::MaterialOptions::COLOR_FACTOR_USED, "COLOR_FACTOR_USED" },
};

void loadTriangleList(data::PrimitiveData &primitive_data, data::Primitive &primitive, SDL_GPUTransferBuffer *transfer_buffer_handle, SDL_GPUCopyPass *copy_pass) {
	//! make no side effect
	primitive.vert_count = primitive_data.vert_count;
	primitive.index_count = primitive_data.index_count;
	primitive.material = primitive_data.material; //! add ref counting
	data::VertAttributes primitive_vert_attrs = data::VertAttributes::NONE;
	std::vector<data::ShaderDefinition> vert_shader_defines;
	for (uint32_t i = 0; i < static_cast<uint32_t>(data::VertAttributeIndex::MAX); i++) {
		if (primitive_data.attrs_data[i].get() != nullptr) {
			data::VertAttributeIndex vert_attr_index = static_cast<data::VertAttributeIndex>(i);
			data::VertAttributes vert_attr = attr_index_map[vert_attr_index];
			vert_shader_defines.push_back({ .name = vert_attr_define_map[vert_attr],
					.value = nullptr });
			primitive_vert_attrs |= vert_attr;
			const uint32_t stride = stride_map[vert_attr_index];
			const uint32_t count = (vert_attr_index == data::VertAttributeIndex::INDEX) ? primitive_data.index_count : primitive_data.vert_count;

			primitive.attrs_data[i].cpu_buffer.swap(primitive_data.attrs_data[i]);

			SDL_GPUBufferCreateInfo buffer_create_info = {
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = stride * count
			};
			primitive.attrs_data[i].gpu_buffer =
					SDL_CreateGPUBuffer(m_GPU_device, &buffer_create_info);
			uint8_t *transfer_buffer = static_cast<uint8_t *>(
					SDL_MapGPUTransferBuffer(m_GPU_device, transfer_buffer_handle, true));
			std::memcpy(transfer_buffer, primitive.attrs_data[i].cpu_buffer.get(), stride * count);
			SDL_UnmapGPUTransferBuffer(m_GPU_device, transfer_buffer_handle);
			SDL_GPUTransferBufferLocation transfer_buffer_location = {
				.transfer_buffer = transfer_buffer_handle,
				.offset = 0
			};
			SDL_GPUBufferRegion buffer_region = {
				.buffer = primitive.attrs_data[i].gpu_buffer,
				.offset = 0,
				.size = stride * count
			};
			SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
		}
	}
	auto material_options = MaS::getMaterialOptions(primitive.material);
	data::PipelineOptions options = {
		.color_target_format = TextureFormat::R8G8B8A8_UNORM,
		.primitive_type = primitive.type,
		.vert_attrs = primitive_vert_attrs,
		.material_options = material_options,
		.vert_shader = { 0, 0 },
		.frag_shader = { 0, 0 },
	};

	options.vert_shader = ShS::createShader(
			GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.hlsl", vert_shader_defines);

	std::vector<data::ShaderDefinition> frag_shader_defines;
	if ((material_options & data::MaterialOptions::COLOR_TEXTURE) == data::MaterialOptions::COLOR_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::COLOR_TEXTURE], .value = nullptr });
	}
	if ((material_options & data::MaterialOptions::EMISSIVE_TEXTURE) == data::MaterialOptions::EMISSIVE_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::EMISSIVE_TEXTURE], .value = nullptr });
	}
	if ((material_options & data::MaterialOptions::NORMAL_TEXTURE) == data::MaterialOptions::NORMAL_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::NORMAL_TEXTURE], .value = nullptr });
	}
	if ((material_options & data::MaterialOptions::METALLIC_ROUGHNESS_TEXTURE) == data::MaterialOptions::METALLIC_ROUGHNESS_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::METALLIC_ROUGHNESS_TEXTURE], .value = nullptr });
	}
	if ((material_options & data::MaterialOptions::OCCLUSION_TEXTURE) == data::MaterialOptions::OCCLUSION_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::OCCLUSION_TEXTURE], .value = nullptr });
	}
	if ((material_options & data::MaterialOptions::COLOR_FACTOR_USED) == data::MaterialOptions::COLOR_FACTOR_USED) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[data::MaterialOptions::COLOR_FACTOR_USED], .value = nullptr });
	}
	options.frag_shader =
			ShS::createShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.frag.hlsl", frag_shader_defines);

	primitive.pipeline = PS::createPipeline(options);
}

}; // namespace

// Mesh Storage
namespace MS {

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
}
void destroy() {
}

handle::Mesh
createMesh(data::MeshData &mesh_data) {
	data::VertAttributes vert_attrs = data::VertAttributes::NONE;
	handle::Mesh mesh_handle = mesh_storage.insert({});
	auto &mesh = mesh_storage.get(mesh_handle);

	uint32_t max_vert_count = 0;
	for (const auto &primitive : mesh_data.primitives) {
		max_vert_count = std::max(max_vert_count, primitive.vert_count);
	}
	if (max_vert_count == 0) {
		return {
			0, 0
		};
	}
	SDL_GPUCommandBuffer *command_buffer =
			SDL_AcquireGPUCommandBuffer(m_GPU_device);
	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	SDL_GPUTransferBufferCreateInfo transfer_create_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = max_stride * max_vert_count
	};
	SDL_GPUTransferBuffer *transfer_buffer_handle =
			SDL_CreateGPUTransferBuffer(m_GPU_device, &transfer_create_info);

	for (auto &primitive : mesh_data.primitives) {
		switch (primitive.type) {
			case data::PrimitiveType::TRIANGLELIST: {
				mesh.primitives.push_back({});
				loadTriangleList(primitive, mesh.primitives.back(), transfer_buffer_handle, copy_pass);
			} break;
			default:
				SDL_assert(false); // Unsupported primitive
		}
	}

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(command_buffer);
	SDL_ReleaseGPUTransferBuffer(m_GPU_device, transfer_buffer_handle);
	return mesh_handle;
}

void refMesh(handle::Mesh mesh) {
	mesh_storage.ref(mesh);
}
void destroyMesh(handle::Mesh mesh) {
	mesh_storage.erase(mesh);
} //! delete mesh

AABB getMeshAABB(handle::Mesh mesh) {
	return mesh_storage.get(mesh).aabb;
}
const data::Primitive &getPrimitiveData(handle::Mesh mesh, uint32_t primitive_index) {
	return mesh_storage.get(mesh).primitives[primitive_index];
}
void setMeshAABB(handle::Mesh mesh, AABB aabb) {
	mesh_storage.get(mesh).aabb = aabb;
	mesh_storage.setIsEdited(mesh);
}
const std::vector<data::Primitive> &getMeshPrimitives(handle::Mesh mesh) {
	return mesh_storage.get(mesh).primitives;
}
}; // namespace MS