#include <filesystem>
#include <renderer/storage/shader-storage.hpp>
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

namespace {

SDL_GPUDevice *m_GPU_device;
ShaderStorageType shader_storage;
Slang::ComPtr<slang::IGlobalSession> globalSession;
}; // namespace

// Shader Storage
namespace ShS {
void init(SDL_GPUDevice *device) {
	m_GPU_device = device;
	createGlobalSession(globalSession.writeRef());
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
	sessionDesc.defaultMatrixLayoutMode = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_ROW_MAJOR;
	std::vector<slang::PreprocessorMacroDesc> preprocessorMacroDesc = {};
	sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
	sessionDesc.preprocessorMacroCount = preprocessorMacroDesc.size();
	slang::CompilerOptionEntry options[] = {
		{ slang::CompilerOptionName::EmitSpirvDirectly,
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr } },
		{ slang::CompilerOptionName::MatrixLayoutRow,
				{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr } }
	};
	sessionDesc.compilerOptionEntries = options;
	sessionDesc.compilerOptionEntryCount = 2;

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

	std::array<slang::IComponentType *, 3> componentTypes = {
		slangModule,
		entryPoint,
		definesModule
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
				}
				else if (kind == slang::TypeReflection::Kind::ParameterBlock) {
					slang::TypeReflection *parBlockElementType = varType->getElementType();
					int fieldCount = parBlockElementType->getFieldCount();
					bool has_uniform_vars = false;
					for (int f = 0; f < fieldCount; f++) {
						slang::VariableReflection *field =
								parBlockElementType->getFieldByIndex(f);
						slang::TypeReflection *fieldType = field->getType();
						slang::TypeReflection::Kind fieldKind = fieldType->getKind();
						if (fieldKind == slang::TypeReflection::Kind::SamplerState) {
							num_samplers++;
						} else if (fieldKind == slang::TypeReflection::Kind::TextureBuffer) {
							num_storage_textures++;
						} else if (fieldKind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
							num_storage_buffers++;
						} else if (fieldKind == slang::TypeReflection::Kind::Struct || fieldKind == slang::TypeReflection::Kind::Array || fieldKind == slang::TypeReflection::Kind::Matrix || fieldKind == slang::TypeReflection::Kind::Vector || fieldKind == slang::TypeReflection::Kind::Scalar) {
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
	shader_data.gpu_handle = SDL_CreateGPUShader(
			m_GPU_device,
			&createinfo);
	SDL_assert(shader_data.gpu_handle != nullptr);
	return shader_storage.insert(shader_data);
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
bool isValid(RE::Shader::Handle shader) {
	return shader_storage.isValid(shader);
}
}; // namespace ShS
