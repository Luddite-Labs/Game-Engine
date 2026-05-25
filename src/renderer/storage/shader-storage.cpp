#include <filesystem>
#include <imgui.h>
#include <renderer/storage/shader-storage.hpp>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

namespace {

SDL_GPUDevice *m_GPU_device;
ShaderStorageType shader_storage;
std::unordered_map<size_t, RE::Shader::Handle> shader_cache;
Slang::ComPtr<slang::IGlobalSession> globalSession;

size_t generateShaderHash(const std::string &path, const std::vector<RE::Shader::Definition> &defines) {
	size_t hash = 0;
	hash_combine(hash, path);
	for (auto &define : defines) {
		hash_combine(hash, define.name);
		hash_combine(hash, define.value);
	}
	return hash;
}
}; // namespace

// Shader Storage
namespace ShS {
void drawShaderDebugUI(RE::Shader::Data &shader_data) {
	ImGui::PushID(reinterpret_cast<size_t>(&shader_data));
	ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;

	if (ImGui::CollapsingHeader(("Shader - " + std::to_string(reinterpret_cast<size_t>(&shader_data))).c_str())) {
		ImGui::TextUnformatted(("Path: " + shader_data.path).c_str());
		ImGui::Text("Shader Type: %s", getString(shader_data.type));

		ImGui::Text("Shader Defines:");
		ImGui::BeginTable("Defines", 2, table_flags);
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Value");
		ImGui::TableHeadersRow();
		for (auto &define : shader_data.defines) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(define.name);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(define.value == nullptr ? "NULL" : define.value);
		}
		ImGui::EndTable();

		ImGui::Text("Shader Resource Metadata:");
		ImGui::BeginTable("Resource Metadata", 2, table_flags);
		ImGui::TableSetupColumn("Resource");
		ImGui::TableSetupColumn("Count");
		ImGui::TableHeadersRow();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Samplers");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(std::to_string(shader_data.num_samplers).c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Storage Textures");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(std::to_string(shader_data.num_storage_textures).c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Storage Buffers");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(std::to_string(shader_data.num_storage_buffers).c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Uniform Buffers");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(std::to_string(shader_data.num_uniform_buffers).c_str());

		ImGui::EndTable();
	}
	ImGui::PopID();
}

void drawShaderDebugUI(RE::Shader::Handle &shader_handle) {
	drawShaderDebugUI(shader_storage.get(shader_handle));
}

void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	createGlobalSession(globalSession.writeRef());
	registerUIDebugCallback("shader-storage", [&]() {
		ImGui::TextUnformatted(("Shader count:" + std::to_string(shader_storage.size())).c_str());
		for (int i = 0; i < shader_storage.size(); i++) {
			drawShaderDebugUI(shader_storage[i]);
		}
	});
}

void destroy() {
	//! destroy global session
}

void diagnoseIfNeeded(slang::IBlob *diagnosticsBlob) {
	if (diagnosticsBlob != nullptr) {
		LOG_ERROR("%s", (const char *)diagnosticsBlob->getBufferPointer());
	}
}

RE::Shader::Handle
createShader(const std::string &shader_file,
		const std::vector<RE::Shader::Definition> &defines) {
	//! migrate to specialization constants
	//! add caching based on defines
	SDL_assert(m_GPU_device != nullptr);

	RE::Shader::Type shader_type;
	if (shader_file.find(".vert") != std::string::npos) {
		shader_type = RE::Shader::Type::VERTEX;
	} else if (shader_file.find(".frag") != std::string::npos) {
		shader_type = RE::Shader::Type::FRAGMENT;
	} else {
		SDL_assert(false); // Unsupported stage
	}

	size_t shader_hash = generateShaderHash(shader_file, defines);
	if (shader_cache.contains(shader_hash)) {
		if (isValid(shader_cache[shader_hash])) {
			refShader(shader_cache[shader_hash]);
			return shader_cache[shader_hash];
		} else {
			shader_cache.erase(shader_hash);
		}
	}

	size_t data_size;
	uint8_t *buffer = static_cast<uint8_t *>(
			SDL_LoadFile(shader_file.c_str(), &data_size));

	Slang::ComPtr<slang::ISession> session;
	slang::SessionDesc sessionDesc = {};
	slang::TargetDesc targetDesc = {}; //! get from SDL in future
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = globalSession->findProfile("spirv_1_5");
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.defaultMatrixLayoutMode = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
	std::vector<slang::PreprocessorMacroDesc> preprocessorMacroDesc = {};
	sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
	sessionDesc.preprocessorMacroCount = preprocessorMacroDesc.size();
	slang::CompilerOptionEntry options[] = {
		{ slang::CompilerOptionName::EmitSpirvDirectly,
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr } },
#ifdef GAME_ENGINE_DEBUG_MODE
		{ slang::CompilerOptionName::DebugInformation,
				{ slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL, 0, nullptr, nullptr } }
#endif
	};
	sessionDesc.compilerOptionEntries = options;
	sessionDesc.compilerOptionEntryCount = 1;

	globalSession->createSession(sessionDesc, session.writeRef());

	std::string defines_module_string = "";
	for (const auto &define : defines) {
		defines_module_string += "export static const bool " + std::string(define.name) + " = true;\n";
	}

	Slang::ComPtr<slang::IModule> definesModule;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		definesModule = session->loadModuleFromSourceString(
				"defines",
				"defines.slang",
				defines_module_string.c_str(),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (!definesModule) {
			exit(1);
		}
	}

	Slang::ComPtr<slang::IModule> commonModule;
	{
		uint8_t *common_buffer = static_cast<uint8_t *>(
				SDL_LoadFile(GAME_ENGINE_DEFAULT_SHADER_DIR "/common.slang", NULL));
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		commonModule = session->loadModuleFromSourceString(
				"common",
				GAME_ENGINE_DEFAULT_SHADER_DIR "/common.slang",
				reinterpret_cast<char *>(common_buffer),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (!commonModule) {
			exit(1);
		}
		SDL_free(common_buffer);
	}

	Slang::ComPtr<slang::IModule> slangModule;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		slangModule = session->loadModuleFromSourceString(
				std::filesystem::path(shader_file).filename().string().c_str(),
				shader_file.c_str(),
				reinterpret_cast<char *>(buffer),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (!slangModule) {
			exit(1);
		}
	}

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		slangModule->findEntryPointByName("main", entryPoint.writeRef());
		if (!entryPoint) {
			exit(1);
		}
	}

	std::array<slang::IComponentType *, 4> componentTypes = {
		commonModule,
		definesModule,
		slangModule,
		entryPoint
	};

	Slang::ComPtr<slang::IComponentType> composedProgram;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		SlangResult result = session->createCompositeComponentType(
				componentTypes.data(),
				componentTypes.size(),
				composedProgram.writeRef(),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (result < 0) {
			exit(1);
		}
	}
	slang::ProgramLayout *programLayout = composedProgram->getLayout();

	Slang::ComPtr<slang::IComponentType> linkedProgram;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		SlangResult result = composedProgram->link(
				linkedProgram.writeRef(),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (result < 0) {
			exit(1);
		}
	}

	Slang::ComPtr<slang::IBlob> spirvCode;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		SlangResult result = linkedProgram->getEntryPointCode(
				0,
				0,
				spirvCode.writeRef(),
				diagnosticsBlob.writeRef());
		diagnoseIfNeeded(diagnosticsBlob);
		if (result < 0) {
			exit(1);
		}
	}

	uint32_t num_samplers = 0;
	uint32_t num_storage_textures = 0;
	uint32_t num_storage_buffers = 0;
	uint32_t num_uniform_buffers = 0;

	uint32_t paramCount = programLayout->getParameterCount();
	for (uint32_t i = 0; i < paramCount; ++i) {
		slang::VariableLayoutReflection *varLayout = programLayout->getParameterByIndex(i);
		if (varLayout) {
			slang::TypeLayoutReflection *typeLayout = varLayout->getTypeLayout();
			if (typeLayout) {
				slang::TypeReflection *varType = typeLayout->getType();
				slang::TypeReflection::Kind kind = varType->getKind();
				if (kind == slang::TypeReflection::Kind::SamplerState) {
					num_samplers++;
				} else if (kind == slang::TypeReflection::Kind::TextureBuffer) {
					num_storage_textures++;
				} else if (kind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
					num_storage_buffers++;
				} else if (kind == slang::TypeReflection::Kind::ConstantBuffer) {
					num_uniform_buffers++;
				} else if (kind == slang::TypeReflection::Kind::ParameterBlock) {
					slang::TypeReflection *parBlockElementType = varType->getElementType();
					int fieldCount = parBlockElementType->getFieldCount();
					bool has_uniform_vars = false;
					for (int f = 0; f < fieldCount; f++) {
						slang::VariableReflection *field =
								parBlockElementType->getFieldByIndex(f);
						auto name = field->getName();
						slang::TypeReflection *fieldType = field->getType();
						slang::TypeReflection::Kind fieldKind = fieldType->getKind();
						if (fieldKind == slang::TypeReflection::Kind::Resource) {
							auto resource_shape = fieldType->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK;
							if (resource_shape == SlangResourceShape::SLANG_TEXTURE_1D || resource_shape == SlangResourceShape::SLANG_TEXTURE_2D) {
								num_samplers++;
							} else if (resource_shape == SlangResourceShape::SLANG_STRUCTURED_BUFFER) {
								num_storage_buffers++;
							}
						} else if (fieldKind == slang::TypeReflection::Kind::SamplerState) {
							num_samplers++;
						} else if (fieldKind == slang::TypeReflection::Kind::TextureBuffer) {
							num_storage_textures++;
						} else if (fieldKind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
							num_storage_buffers++;
						} else if (fieldKind == slang::TypeReflection::Kind::Struct || fieldKind == slang::TypeReflection::Kind::Array || fieldKind == slang::TypeReflection::Kind::Matrix || fieldKind == slang::TypeReflection::Kind::Vector || fieldKind == slang::TypeReflection::Kind::Scalar && std::string_view(field->getName()) != "pad") {
							has_uniform_vars = true;
						}
					}
					num_uniform_buffers += static_cast<int>(has_uniform_vars);
				}
			}
		}
	}

	paramCount = entryPoint->getLayout()->getParameterCount();
	for (uint32_t i = 0; i < paramCount; ++i) {
		slang::VariableLayoutReflection *varLayout = entryPoint->getLayout()->getParameterByIndex(i);
		if (varLayout) {
			slang::TypeLayoutReflection *typeLayout = varLayout->getTypeLayout();
			if (typeLayout) {
				slang::TypeReflection *varType = typeLayout->getType();
				slang::TypeReflection::Kind kind = varType->getKind();
				if (kind == slang::TypeReflection::Kind::SamplerState) {
					num_samplers++;
				} else if (kind == slang::TypeReflection::Kind::TextureBuffer) {
					num_storage_textures++;
				} else if (kind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
					num_storage_buffers++;
				} else if (kind == slang::TypeReflection::Kind::ConstantBuffer) {
					num_uniform_buffers++;
				} else if (kind == slang::TypeReflection::Kind::ParameterBlock) {
					slang::TypeReflection *parBlockElementType = varType->getElementType();
					int fieldCount = parBlockElementType->getFieldCount();
					bool has_uniform_vars = false;
					for (int f = 0; f < fieldCount; f++) {
						slang::VariableReflection *field =
								parBlockElementType->getFieldByIndex(f);
						slang::TypeReflection *fieldType = field->getType();
						slang::TypeReflection::Kind fieldKind = fieldType->getKind();
						if (kind == slang::TypeReflection::Kind::SamplerState) {
							num_samplers++;
						} else if (kind == slang::TypeReflection::Kind::TextureBuffer) {
							num_storage_textures++;
						} else if (kind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
							num_storage_buffers++;
						} else if (kind == slang::TypeReflection::Kind::Struct || kind == slang::TypeReflection::Kind::Array || kind == slang::TypeReflection::Kind::Matrix || kind == slang::TypeReflection::Kind::Vector || kind == slang::TypeReflection::Kind::Scalar) {
							bool has_uniform_vars = true;
						}
					}
					num_uniform_buffers += static_cast<int>(has_uniform_vars);
				}
			}
		}
	}

	RE::Shader::Data shader_data = {};
	CHECK_AND_PRINT_SDL_ERROR();
	SDL_GPUShaderCreateInfo createinfo = {
		.code_size = spirvCode->getBufferSize(),
		.code = reinterpret_cast<const uint8_t *>(spirvCode->getBufferPointer()),
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = static_cast<SDL_GPUShaderStage>(shader_type),
		.num_samplers = num_samplers,
		.num_storage_textures = num_storage_textures,
		.num_storage_buffers = num_storage_buffers,
		.num_uniform_buffers = num_uniform_buffers,
	};
	shader_data.num_samplers = num_samplers,
	shader_data.num_storage_textures = num_storage_textures,
	shader_data.num_storage_buffers = num_storage_buffers,
	shader_data.num_uniform_buffers = num_uniform_buffers,
	shader_data.defines = defines;
	shader_data.path = shader_file;
	shader_data.gpu_handle = SDL_CreateGPUShader(
			m_GPU_device,
			&createinfo);
	SDL_assert(shader_data.gpu_handle != nullptr);
	auto shader_handle = shader_storage.insert(shader_data);
	shader_cache[shader_hash] = shader_handle;
	return shader_handle;
}
void refShader(RE::Shader::Handle shader) {
	shader_storage.ref(shader);
}
void destroyShader(RE::Shader::Handle shader) {
	shader_storage.erase(shader);
}
RE::Shader::Type getShaderType(RE::Shader::Handle shader) {
	return shader_storage.get(shader).type;
}
// const char *getShaderFilePath(RE::Shader::Handle shader);
uint32_t getShaderNumSamplers(RE::Shader::Handle shader) {
	return shader_storage.get(shader).num_samplers;
}
uint32_t getShaderNumStorageTextures(RE::Shader::Handle shader) {
	return shader_storage.get(shader).num_storage_textures;
}
uint32_t getShaderNumStorageBuffers(RE::Shader::Handle shader) {
	return shader_storage.get(shader).num_storage_buffers;
}
uint32_t getShaderNumUniformBuffers(RE::Shader::Handle shader) {
	return shader_storage.get(shader).num_uniform_buffers;
}
SDL_GPUShader *getShaderGPUHandle(RE::Shader::Handle shader) {
	return shader_storage.get(shader).gpu_handle;
}
const std::vector<RE::Shader::Definition> &getShaderDefines(RE::Shader::Handle shader) {
	return shader_storage.get(shader).defines;
}
std::string getShaderPath(RE::Shader::Handle shader) {
	return shader_storage.get(shader).path;
}
bool isValid(RE::Shader::Handle shader) {
	return shader_storage.isValid(shader);
}
}; // namespace ShS
