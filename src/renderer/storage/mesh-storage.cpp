#include <imgui.h>
#include <renderer/storage/material-storage.hpp>
#include <renderer/storage/mesh-storage.hpp>
#include <renderer/storage/pipeline-storage.hpp>
#include <renderer/storage/shader-storage.hpp>

namespace {
SDL_GPUDevice *m_GPU_device;
MeshStorageType mesh_storage;
uint32_t max_stride = sizeof(glm::vec4);
std::unordered_map<RE::Vertex::AttributeIndex, uint32_t> stride_map = {
	{ RE::Vertex::AttributeIndex::POSITION, sizeof(glm::vec3) },
	{ RE::Vertex::AttributeIndex::NORMAL, sizeof(glm::vec3) },
	{ RE::Vertex::AttributeIndex::TANGENT, sizeof(glm::vec4) },
	{ RE::Vertex::AttributeIndex::COLOR, sizeof(glm::vec4) },
	{ RE::Vertex::AttributeIndex::UV, sizeof(glm::vec2) },
	{ RE::Vertex::AttributeIndex::INDEX, sizeof(uint16_t) },
};
std::unordered_map<RE::Vertex::AttributeIndex, RE::Vertex::Attributes> attr_index_map = {
	{ RE::Vertex::AttributeIndex::POSITION, RE::Vertex::Attributes::POSITION },
	{ RE::Vertex::AttributeIndex::NORMAL, RE::Vertex::Attributes::NORMAL },
	{ RE::Vertex::AttributeIndex::TANGENT, RE::Vertex::Attributes::TANGENT },
	{ RE::Vertex::AttributeIndex::COLOR, RE::Vertex::Attributes::COLOR },
	{ RE::Vertex::AttributeIndex::UV, RE::Vertex::Attributes::UV },
	{ RE::Vertex::AttributeIndex::INDEX, RE::Vertex::Attributes::INDEX },
};

std::unordered_map<RE::Vertex::Attributes, const char *> vert_attr_define_map = {
	{ RE::Vertex::Attributes::POSITION, "POSITION_USED" },
	{ RE::Vertex::Attributes::NORMAL, "NORMAL_USED" },
	{ RE::Vertex::Attributes::TANGENT, "TANGENT_USED" },
	{ RE::Vertex::Attributes::COLOR, "COLOR_USED" },
	{ RE::Vertex::Attributes::UV, "UV_USED" },
	{ RE::Vertex::Attributes::INDEX, "INDEX_USED" },
};

std::unordered_map<RE::Material::Options, const char *> frag_option_define_map = {
	{ RE::Material::Options::COLOR_TEXTURE, "COLOR_TEXTURE_USED" },
	{ RE::Material::Options::EMISSIVE_TEXTURE, "EMISSIVE_TEXTURE_USED" },
	{ RE::Material::Options::NORMAL_TEXTURE, "NORMAL_TEXTURE_USED" },
	{ RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE, "METALLIC_ROUGHNESS_TEXTURE_USED" },
	{ RE::Material::Options::OCCLUSION_TEXTURE, "OCCLUSION_TEXTURE_USED" },
	{ RE::Material::Options::COLOR_FACTOR_USED, "COLOR_FACTOR_USED" },
};

void loadTriangleList(RE::Mesh::Primitive::Arg &primitive_data, RE::Mesh::Primitive::Data &primitive, SDL_GPUTransferBuffer *transfer_buffer_handle, SDL_GPUCopyPass *copy_pass) {
	//! make no side effect
	primitive.vert_count = primitive_data.vert_count;
	primitive.index_count = primitive_data.index_count;
	primitive.material = primitive_data.material;
	MaS::refMaterial(primitive.material);
	RE::Vertex::Attributes primitive_vert_attrs = RE::Vertex::Attributes::NONE;
	std::vector<RE::Shader::Definition> vert_shader_defines;
	std::vector<RE::Shader::Definition> frag_shader_defines;
	for (uint32_t i = 0; i < static_cast<uint32_t>(RE::Vertex::AttributeIndex::MAX); i++) {
		if (primitive_data.attrs_data[i].get() != nullptr) {
			RE::Vertex::AttributeIndex vert_attr_index = static_cast<RE::Vertex::AttributeIndex>(i);
			RE::Vertex::Attributes vert_attr = attr_index_map[vert_attr_index];
			vert_shader_defines.push_back({ .name = vert_attr_define_map[vert_attr],
					.value = nullptr });
			frag_shader_defines.push_back({ .name = vert_attr_define_map[vert_attr],
					.value = nullptr });
			primitive_vert_attrs |= vert_attr;
			uint32_t stride = stride_map[vert_attr_index];
			if (vert_attr_index == RE::Vertex::AttributeIndex::INDEX) {
				stride = (primitive_data.index_count <= UINT16_MAX) ? sizeof(uint16_t) : sizeof(uint32_t);
			}
			const uint32_t count = (vert_attr_index == RE::Vertex::AttributeIndex::INDEX) ? primitive_data.index_count : primitive_data.vert_count;

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
	RE::Pipeline::Options options = {
		.color_target_format = RE::Texture::Format::R8G8B8A8_UNORM, //! get from color_target
		.primitive_type = primitive.type,
		.vert_attrs = primitive_vert_attrs,
		.material_options = material_options,
		.vert_shader = { 0, 0 },
		.frag_shader = { 0, 0 },
		.disable_back_face_culling = MaS::getDoubleSided(primitive.material),
		.disable_depth_write=false
	}; // ! instanced

	if (MaS::getMaterialAlphaMode(primitive.material) == RE::Material::AlphaModes::BLEND) {
		options.vert_shader = ShS::createShader(
				GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.slang", vert_shader_defines);
	} else {
		options.vert_shader = ShS::createShader(
				GAME_ENGINE_DEFAULT_SHADER_DIR "/base.vert.slang", vert_shader_defines);
	}
	if ((material_options & RE::Material::Options::COLOR_TEXTURE) == RE::Material::Options::COLOR_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::COLOR_TEXTURE], .value = nullptr });
	}
	if ((material_options & RE::Material::Options::EMISSIVE_TEXTURE) == RE::Material::Options::EMISSIVE_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::EMISSIVE_TEXTURE], .value = nullptr });
	}
	if ((material_options & RE::Material::Options::NORMAL_TEXTURE) == RE::Material::Options::NORMAL_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::NORMAL_TEXTURE], .value = nullptr });
	}
	if ((material_options & RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE) == RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::METALLIC_ROUGHNESS_TEXTURE], .value = nullptr });
	}
	if ((material_options & RE::Material::Options::OCCLUSION_TEXTURE) == RE::Material::Options::OCCLUSION_TEXTURE) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::OCCLUSION_TEXTURE], .value = nullptr });
	}
	if ((material_options & RE::Material::Options::COLOR_FACTOR_USED) == RE::Material::Options::COLOR_FACTOR_USED) {
		frag_shader_defines.push_back({ .name = frag_option_define_map[RE::Material::Options::COLOR_FACTOR_USED], .value = nullptr });
	}
	if (MaS::getMaterialAlphaMode(primitive.material) == RE::Material::AlphaModes::BLEND) {
		options.disable_depth_write	= true;
		options.frag_shader =
				ShS::createShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/alpha.frag.slang", frag_shader_defines);
	} else {
		options.frag_shader =
				ShS::createShader(GAME_ENGINE_DEFAULT_SHADER_DIR "/base.frag.slang", frag_shader_defines);
	}

	primitive.pipeline = PS::createPipeline(options);
} // namespace
}; // namespace

// Mesh Storage
namespace MS {

void drawMeshDebugUI(RE::Mesh::Data &mesh_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&mesh_data));
	for (int i = 0; i < mesh_data.primitives.size(); i++) {
		if (ImGui::CollapsingHeader(("Primitive - " + std::to_string(i)).c_str())) {
			ImGui::Text("Vert Count: %d", mesh_data.primitives[i].vert_count);
			ImGui::Text("Index Count: %d", mesh_data.primitives[i].index_count);
			ImGui::Text("Primitive Type: %s", getString(mesh_data.primitives[i].type));
			ImGui::Indent();
			MaS::drawMaterialDebugUI(mesh_data.primitives[i].material);
			ImGui::Unindent();
		}
	}
	ImGui::PopID();
}

void drawMeshDebugUI(RE::Mesh::Handle &mesh_handle) {
	drawMeshDebugUI(mesh_storage.get(mesh_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	registerUIDebugCallback("mesh-storage", [&]() {
		ImGui::TextUnformatted(("Mesh count:" + std::to_string(mesh_storage.size())).c_str());
	});
}
void destroy() {
}

RE::Mesh::Handle
createMesh(RE::Mesh::Arg &mesh_data) {
	RE::Vertex::Attributes vert_attrs = RE::Vertex::Attributes::NONE;
	uint32_t max_vert_count = 0;
	for (const auto &primitive : mesh_data.primitives) {
		max_vert_count = std::max(max_vert_count, primitive.vert_count);
	}
	if (max_vert_count == 0) {
		return {
			0, 0
		};
	}

	RE::Mesh::Handle mesh_handle = mesh_storage.insert({});
	auto &mesh = mesh_storage.get(mesh_handle);

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
			case RE::Mesh::Primitive::Type::TRIANGLELIST: {
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

void refMesh(RE::Mesh::Handle mesh) {
	mesh_storage.ref(mesh);
}
void destroyMesh(RE::Mesh::Handle mesh) {
	mesh_storage.erase(mesh);
} //! delete mesh

RE::Mesh::AABB getMeshAABB(RE::Mesh::Handle mesh) {
	return mesh_storage.get(mesh).aabb;
}
const RE::Mesh::Primitive::Data &getPrimitiveData(RE::Mesh::Handle mesh, uint32_t primitive_index) {
	return mesh_storage.get(mesh).primitives[primitive_index];
}
void setMeshAABB(RE::Mesh::Handle mesh, RE::Mesh::AABB aabb) {
	mesh_storage.get(mesh).aabb = aabb;
	mesh_storage.setIsEdited(mesh);
}
const std::vector<RE::Mesh::Primitive::Data> &getMeshPrimitives(RE::Mesh::Handle mesh) {
	return mesh_storage.get(mesh).primitives;
}
bool isValid(RE::Mesh::Handle mesh) {
	return mesh_storage.isValid(mesh);
}
}; // namespace MS