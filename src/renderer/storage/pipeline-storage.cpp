#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_gpu.h"
#include "renderer/types.hpp"
#include <imgui.h>
#include <misc/utils.hpp>
#include <renderer/storage/pipeline-storage.hpp>
#include <renderer/storage/shader-storage.hpp>
#include <unordered_map>

template <>
struct std::hash<RE::Pipeline::Options> {
	size_t operator()(const RE::Pipeline::Options &p) const {
		size_t hash = 0;
		hash |= p.vert_shader.slot_index;
		hash <<= 16;
		uint16_t lossy_frag_shader =
				static_cast<uint16_t>(p.frag_shader.slot_index);
		hash |= lossy_frag_shader;
		hash <<= 16;
		hash |= static_cast<uint8_t>(p.material_options);
		hash <<= 8;
		hash |= static_cast<uint8_t>(p.vert_attrs);
		hash <<= 8;
		hash |= static_cast<uint8_t>(p.primitive_type);
		hash <<= 8;
		hash |= static_cast<uint8_t>(p.color_target_format);
		hash <<= 8;
		return hash;
	}
};

namespace {
using PipelineCache = std::unordered_map<RE::Pipeline::Options, RE::Pipeline::Handle>;

SDL_GPUDevice *m_GPU_device;
PipelineStorageType pipeline_storage;
PipelineCache pipeline_cache;
}; // namespace

// Pipeline Storage
namespace PS {

void drawPipelineDebugUI(RE::Pipeline::Data &pipeline_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&pipeline_data));
	if (ImGui::CollapsingHeader(("pipeline - " + std::to_string(reinterpret_cast<size_t>(&pipeline_data))).c_str())) {
		ImGui::TextUnformatted("Options:");

		ImGui::TextUnformatted("Color Target Format");
		ImGui::SameLine();
		ImGui::TextUnformatted(getString(pipeline_data.options.color_target_format));

		ImGui::TextUnformatted("Primitive Type:");
		ImGui::SameLine();
		ImGui::TextUnformatted(getString(pipeline_data.options.primitive_type));

		ImGui::TextUnformatted("Vertex attributes:");
		for (int i = 0; i < static_cast<int>(RE::Vertex::AttributeIndex::MAX); i++) {
			if ((1 << i) & static_cast<int>(pipeline_data.options.vert_attrs)) {
				ImGui::Indent();
				ImGui::TextUnformatted(getString(static_cast<RE::Vertex::Attributes>(1 << i)));
				ImGui::Unindent();
			}
		}

		ImGui::TextUnformatted("Material options:");
		for (int i = 0; i < 6; i++) {
			if ((1 << i) & static_cast<int>(pipeline_data.options.material_options)) {
				ImGui::Indent();
				ImGui::TextUnformatted(getString(static_cast<RE::Material::Options>(1 << i)));
				ImGui::Unindent();
			}
		}
		ShS::drawShaderDebugUI(pipeline_data.options.vert_shader);
		ShS::drawShaderDebugUI(pipeline_data.options.frag_shader);

		ImGui::BeginDisabled();
		ImGui::Checkbox("Disable back face culling:", &pipeline_data.options.disable_back_face_culling);
		ImGui::EndDisabled();

		ImGui::BeginDisabled();
		ImGui::Checkbox("Instanced:", &pipeline_data.options.instanced);
		ImGui::EndDisabled();
	}
	ImGui::PopID();
}

void drawPipelineDebugUI(const RE::Pipeline::Handle &pipeline_handle) {
	drawPipelineDebugUI(pipeline_storage.get(pipeline_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	registerUIDebugCallback("pipeline-storage", [&]() {
		ImGui::TextUnformatted(("Pipeline count:" + std::to_string(pipeline_storage.size())).c_str());
		for (int i = 0; i < pipeline_storage.size(); i++) {
			drawPipelineDebugUI(pipeline_storage[i]);
		}
	});
}
void destroy() {}
RE::Pipeline::Handle createPipeline(const RE::Pipeline::Options &options) {
	if (pipeline_cache.find(options) != pipeline_cache.end()) {
		return pipeline_cache[options];
	}

	SDL_GPUColorTargetDescription color_target_descriptions[] = {
		{ .format =
						static_cast<SDL_GPUTextureFormat>(options.color_target_format),
				.blend_state = {
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
						.enable_blend = true,
				} }
	};
	SDL_GPUGraphicsPipelineTargetInfo target_info = {};
	target_info.color_target_descriptions = color_target_descriptions;
	target_info.num_color_targets = 1;
	if (options.primitive_type == RE::Mesh::Primitive::Type::TRIANGLELIST) {
		target_info.depth_stencil_format = static_cast<SDL_GPUTextureFormat>(RE::Texture::Format::D24_UNORM);
		target_info.has_depth_stencil_target = true;
	}
	std::vector<SDL_GPUVertexAttribute>
			vert_attrs; //! specialise accross primitve type check ate least one
						//! vert attr present
	if ((options.vert_attrs & RE::Vertex::Attributes::POSITION) ==
			RE::Vertex::Attributes::POSITION) {
		vert_attrs.push_back({ .location = static_cast<uint8_t>(RE::Vertex::AttributeIndex::POSITION),
				.buffer_slot = static_cast<uint8_t>(RE::Vertex::AttributeIndex::POSITION),
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.offset = 0 });
	}
	if ((options.vert_attrs & RE::Vertex::Attributes::NORMAL) ==
			RE::Vertex::Attributes::NORMAL) {
		vert_attrs.push_back({ .location = static_cast<uint8_t>(RE::Vertex::AttributeIndex::NORMAL),
				.buffer_slot = static_cast<uint8_t>(RE::Vertex::AttributeIndex::NORMAL),
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.offset = 0 });
	}
	if ((options.vert_attrs & RE::Vertex::Attributes::TANGENT) ==
			RE::Vertex::Attributes::TANGENT) {
		vert_attrs.push_back({ .location = static_cast<uint8_t>(RE::Vertex::AttributeIndex::TANGENT),
				.buffer_slot = static_cast<uint8_t>(RE::Vertex::AttributeIndex::TANGENT),
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				.offset = 0 });
	}
	if ((options.vert_attrs & RE::Vertex::Attributes::COLOR) ==
			RE::Vertex::Attributes::COLOR) {
		vert_attrs.push_back({ .location = static_cast<uint8_t>(RE::Vertex::AttributeIndex::COLOR),
				.buffer_slot = static_cast<uint8_t>(RE::Vertex::AttributeIndex::COLOR),
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				.offset = 0 });
	}
	if ((options.vert_attrs & RE::Vertex::Attributes::UV) ==
			RE::Vertex::Attributes::UV) {
		vert_attrs.push_back({ .location = static_cast<uint8_t>(RE::Vertex::AttributeIndex::UV),
				.buffer_slot = static_cast<uint8_t>(RE::Vertex::AttributeIndex::UV),
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.offset = 0 });
	}
	SDL_GPUVertexInputRate input_rate;
	if (options.instanced) {
		input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
	} else {
		input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	}
	std::vector<SDL_GPUVertexBufferDescription> vert_buffer_descriptions;
	for (const auto &vert_attr : vert_attrs) {
		uint32_t pitch = 0;
		switch (vert_attr.format) {
			case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
				pitch = sizeof(float) * 4;
				break;
			case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
				pitch = sizeof(float) * 3;
				break;
			case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
				pitch = sizeof(float) * 2;
				break;
			default: //! make format and enum and support all enum types
				pitch = 1;
				break;
		}
		vert_buffer_descriptions.push_back({
				.slot = vert_attr.buffer_slot,
				.pitch = pitch,
				.input_rate = input_rate,
				.instance_step_rate = 0,
		});
	}
	auto vert_shader = options.vert_shader;
	auto frag_shader = options.frag_shader;
	SDL_assert(ShS::isValid(vert_shader));
	SDL_assert(ShS::isValid(frag_shader));
	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.vertex_shader =
			ShS::getShaderGPUHandle(vert_shader);
	pipelineCreateInfo.fragment_shader =
			ShS::getShaderGPUHandle(frag_shader);
	pipelineCreateInfo.vertex_input_state = {
		.vertex_buffer_descriptions = vert_buffer_descriptions.data(),
		.num_vertex_buffers = static_cast<uint32_t>(vert_buffer_descriptions.size()),
		.vertex_attributes = vert_attrs.data(),
		.num_vertex_attributes = static_cast<uint32_t>(vert_attrs.size())
	};
	pipelineCreateInfo.primitive_type =
			static_cast<SDL_GPUPrimitiveType>(options.primitive_type);
	pipelineCreateInfo.rasterizer_state = {
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = (options.disable_back_face_culling) ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK,
		.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
	};
	if (options.primitive_type == RE::Mesh::Primitive::Type::TRIANGLELIST) {
		pipelineCreateInfo.depth_stencil_state = {
			.compare_op = SDL_GPU_COMPAREOP_LESS,
			.write_mask = 0xFF,
			.enable_depth_test = true,
			.enable_depth_write = true,
			.enable_stencil_test = false,
		};
	}
	pipelineCreateInfo.target_info = target_info;

	SDL_GPUGraphicsPipeline *gpu_handle =
			SDL_CreateGPUGraphicsPipeline(m_GPU_device, &pipelineCreateInfo);
	RE::Pipeline::Handle pipeline_handle =
			pipeline_storage.insert({ .gpu_handle = gpu_handle, .options = options });
	pipeline_cache[options] = pipeline_handle;
	return pipeline_handle;
}

SDL_GPUGraphicsPipeline *getPipelineGPUHandle(RE::Pipeline::Handle pipeline) {
	SDL_assert(pipeline_storage.isValid(pipeline));
	return pipeline_storage.get(pipeline).gpu_handle;
}
bool isValid(RE::Pipeline::Handle pipeline) {
	return pipeline_storage.isValid(pipeline);
}
}; // namespace PS