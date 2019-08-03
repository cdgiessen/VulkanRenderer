#include "Shader.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <utility>

#include <nlohmann/json.hpp>

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Public/ShaderLang.h>

#include "Device.h"
#include "Initializers.h"
#include "RenderTools.h"
#include "core/CoreTools.h"
#include "core/JobSystem.h"
#include "core/Logger.h"

#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"

const TLimits DefaultTBuiltInLimits = {
	/* .nonInductiveForLoops = */ 1,
	/* .whileLoops = */ 1,
	/* .doWhileLoops = */ 1,
	/* .generalUniformIndexing = */ 1,
	/* .generalAttributeMatrixVectorIndexing = */ 1,
	/* .generalVaryingIndexing = */ 1,
	/* .generalSamplerIndexing = */ 1,
	/* .generalVariableIndexing = */ 1,
	/* .generalConstantMatrixVectorIndexing = */ 1,
};

const TBuiltInResource DefaultTBuiltInResource = { /* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4 };

ShaderType GetShaderStage (const std::string& stage)
{
	if (stage == "vert" || stage == ".vert")
	{
		return ShaderType::vertex;
	}
	else if (stage == "tesc" || stage == ".tesc")
	{
		return ShaderType::tessControl;
	}
	else if (stage == "tese" || stage == ".tese")
	{
		return ShaderType::tessEval;
	}
	else if (stage == "geom" || stage == ".geom")
	{
		return ShaderType::geometry;
	}
	else if (stage == "frag" || stage == ".frag")
	{
		return ShaderType::fragment;
	}
	else if (stage == "comp" || stage == ".comp")
	{
		return ShaderType::compute;
	}
	else
	{
		assert (0 && "Unknown shader stage");
		return ShaderType::error;
	}
}


std::vector<uint32_t> load_spirv_file () { return std::vector<uint32_t> (2); }

void test_func ()
{
	// Read SPIR-V from disk or similar.
	std::vector<uint32_t> spirv_binary = load_spirv_file ();

	spirv_cross::CompilerGLSL glsl (std::move (spirv_binary));

	// The SPIR-V is now parsed, and we can perform reflection on it.
	spirv_cross::ShaderResources resources = glsl.get_shader_resources ();

	// Get all sampled images in the shader.
	for (auto& resource : resources.sampled_images)
	{
		unsigned set = glsl.get_decoration (resource.id, spv::DecorationDescriptorSet);
		unsigned binding = glsl.get_decoration (resource.id, spv::DecorationBinding);
		printf ("Image %s at set = %u, binding = %u\n", resource.name.c_str (), set, binding);

		// Modify the decoration to prepare it for GLSL.
		glsl.unset_decoration (resource.id, spv::DecorationDescriptorSet);

		// Some arbitrary remapping if we want.
		glsl.set_decoration (resource.id, spv::DecorationBinding, set * 16 + binding);
	}

	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
	options.version = 310;
	options.es = true;
	glsl.set_common_options (options);

	// Compile to GLSL, ready to give to GL driver.
	std::string source = glsl.compile ();
}
ShaderModule::ShaderModule () {}
ShaderModule::ShaderModule (ShaderType type, VkShaderModule module) : type (type), module (module)
{
}

VkPipelineShaderStageCreateInfo ShaderModule::GetCreateInfo ()
{
	switch (type)
	{
		case (ShaderType::vertex):
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_VERTEX_BIT, module);
		case (ShaderType::fragment):
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_FRAGMENT_BIT, module);
		case (ShaderType::geometry):
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_GEOMETRY_BIT, module);
		case (ShaderType::tessEval):
			return initializers::pipelineShaderStageCreateInfo (
			    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, module);
		case (ShaderType::tessControl):
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, module);
		case (ShaderType::compute):
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_COMPUTE_BIT, module);
		case (ShaderType::error):
			Log.Error ("Shader module type not correct");
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_VERTEX_BIT, module);
	}
};

ShaderModuleSet::ShaderModuleSet (){};

ShaderModuleSet& ShaderModuleSet::Vertex (ShaderModule vert)
{
	this->vert = vert;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::Fragment (ShaderModule frag)
{
	this->frag = frag;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::Geometry (ShaderModule geom)
{
	this->geom = geom;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::TessControl (ShaderModule tesc)
{
	this->tesc = tesc;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::TessEval (ShaderModule tese)
{
	this->tese = tese;
	return *this;
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderModuleSet::ShaderStageCreateInfos ()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	if (vert.has_value ()) shaderStages.push_back (vert->GetCreateInfo ());
	if (frag.has_value ()) shaderStages.push_back (frag->GetCreateInfo ());
	if (geom.has_value ()) shaderStages.push_back (geom->GetCreateInfo ());
	if (tesc.has_value ()) shaderStages.push_back (tesc->GetCreateInfo ());
	if (tese.has_value ()) shaderStages.push_back (tese->GetCreateInfo ());

	return shaderStages;
}

// time_t LastTimeWritten (std::filesystem::path entry)
//{
//	auto timeEntry = std::filesystem::last_write_time (entry);
//	return std::chrono::system_clock::to_time_t (timeEntry);
//}

void to_json (nlohmann::json& j, const ShaderDatabase::DBHandle& handle)
{
	j = nlohmann::json ({ { "filename", handle.filename },
	    { "type", static_cast<int> (handle.type) },
	    { "glsl_last_write_time", handle.glsl_last_write_time.time_since_epoch ().count () },
	    { "spirv_last_write_time", handle.spirv_last_write_time.time_since_epoch ().count () } });
}

void from_json (const nlohmann::json& j, ShaderDatabase::DBHandle& handle)
{
	j.at ("filename").get_to (handle.filename);
	j.at ("type").get_to (handle.type);

	auto glsl_time = std::chrono::time_point_cast<std::chrono::milliseconds> (handle.glsl_last_write_time)
	                     .time_since_epoch ()
	                     .count ();
	j.at ("glsl_last_write_time").get_to (glsl_time);

	auto spirv_time = std::chrono::time_point_cast<std::chrono::milliseconds> (handle.spirv_last_write_time)
	                      .time_since_epoch ()
	                      .count ();
	j.at ("spirv_last_write_time").get_to (spirv_time);
}

ShaderDatabase::ShaderDatabase () : fileWatch ("assets/shaders")
{
	Load ();
	Refresh ();
}
void ShaderDatabase::Load ()
{
	if (fileExists (database_path))
	{
		try
		{
			std::ifstream input (database_path);
			nlohmann::json j;

			input >> j;
			entries.reserve (j["shader_count"]);
			if (j["shader_count"] > 0)
			{
				for (auto& entry : j)
				{
					entries.push_back (entry);
				}
			}
		}
		catch (nlohmann::detail::parse_error e)
		{
			Log.Debug ("Shader Database file was bas, creating a new one\n");
			Save ();
		}
		catch (std::runtime_error e)
		{
			Log.Debug ("Shader Database file was incorrect, creating a new one\n");
			Save ();
		}
	}
	else
	{
		Log.Debug ("Shader Database doesn't exist, creating one\n");
		Save ();
	}
}

void ShaderDatabase::Save ()
{
	nlohmann::json j;

	for (auto& entry : entries)
	{
		j[entry.filename] = entry;
	}

	j["shader_count"] = entries.size ();

	std::ofstream outFile (database_path);
	outFile << std::setw (4) << j;
	outFile.close ();
}

void ShaderDatabase::Refresh ()
{
	namespace fs = std::filesystem;
	for (auto& e : entries)
	{
		fs::path glsl_path = shader_path + e.filename;
		e.glsl_last_write_time = fs::last_write_time (glsl_path);

		fs::path spirv_path = shader_path + e.filename + ".spv";
		e.spirv_last_write_time = fs::last_write_time (spirv_path);
	}
}


void ShaderDatabase::Discover ()
{
	namespace fs = std::filesystem;

	std::vector<fs::path> glsl_paths;
	std::vector<fs::path> spirv_paths;

	for (auto const& entry : fs::directory_iterator (shader_path))
	{
		auto stem = entry.path ().stem ().string ();
		auto ext = entry.path ().extension ().string ();
		if (ext == ".spv")
		{
			spirv_paths.push_back (entry);
		}
		if (ext == ".vert" || ext == ".frag" || ext == ".geom" || ext == ".tesc" || ext == ".tese" || ext == ".comp")
		{
			glsl_paths.push_back (entry);
		}
	}
}

static std::atomic_bool is_setup = false;

ShaderCompiler::ShaderCompiler ()
{
	if (!is_setup)
	{
		is_setup = true;
		glslang::InitializeProcess ();
	}
}

// Load GLSL into a string
std::optional<std::string> ShaderCompiler::load_file_data (const std::string& filename)
{
	std::ifstream file (filename);

	if (!file.is_open ())
	{
		Log.Error (fmt::format ("Failed to load shader: {}", filename));
		return {};
	}

	return std::string ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
}

std::optional<std::vector<uint32_t>> const ShaderCompiler::compile_glsl_to_spriv (std::string const& shader_name,
    std::string const& shader_data,
    ShaderType const shader_type,
    std::filesystem::path include_path)
{
	const char* InputCString = shader_data.c_str ();

	glslang::TShader Shader (static_cast<EShLanguage> (shader_type));

	Shader.setStrings (&InputCString, 1);

	int ClientInputSemanticsVersion = 110; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
	glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

	Shader.setEnvInput (
	    glslang::EShSourceGlsl, static_cast<EShLanguage> (shader_type), glslang::EShClientVulkan, ClientInputSemanticsVersion);
	Shader.setEnvClient (glslang::EShClientVulkan, VulkanClientVersion);
	Shader.setEnvTarget (glslang::EShTargetSpv, TargetVersion);

	TBuiltInResource Resources;
	Resources = DefaultTBuiltInResource;
	Resources.limits = DefaultTBuiltInLimits;

	EShMessages messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);

	const int DefaultVersion = 100;

	DirStackFileIncluder Includer;

	if (!include_path.empty ())
	{
		Includer.pushExternalLocalDirectory (include_path.string ());
	}

	std::string PreprocessedGLSL;

	if (!Shader.preprocess (&Resources, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer))
	{
		Log.Error (fmt::format ("GLSL Preprocessing Failed for: {}\n", shader_name));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	const char* PreprocessedCStr = PreprocessedGLSL.c_str ();
	Shader.setStrings (&PreprocessedCStr, 1);

	if (!Shader.parse (&Resources, 100, false, messages))
	{
		Log.Error (fmt::format ("GLSL Parsing Failed for: {}\n", shader_name));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	glslang::TProgram Program;
	Program.addShader (&Shader);

	if (!Program.link (messages))
	{
		Log.Error (fmt::format ("GLSL Linking Failed for: {}\n", shader_name));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	std::vector<uint32_t> SpirV;
	spv::SpvBuildLogger logger;
	glslang::SpvOptions spvOptions;
	glslang::GlslangToSpv (
	    *Program.getIntermediate (static_cast<EShLanguage> (shader_type)), SpirV, &logger, &spvOptions);

	return SpirV;
}

// void CompileShaders (std::vector<std::string> filenames)
// {
// 	unsigned int cts = std::thread::hardware_concurrency ();

// 	std::vector<std::vector<std::string>> buckets (cts);
// 	for (size_t i = 0; i < filenames.size (); i++)
// 	{
// 		int index = i % cts;
// 		buckets.at (index).push_back (filenames.at (i));
// 	}

// 	auto signal = std::make_shared<job::TaskSignal> ();

// 	std::vector<job::Task> tasks;
// 	for (auto& bucket : buckets)
// 	{
// 		tasks.push_back (job::Task (signal, [&]() { StartShaderCompilation (bucket); }));
// 	}
// 	taskManager.Submit (tasks, job::TaskType::currentFrame);

// 	signal->Wait ();
// }

ShaderManager::ShaderManager (VulkanDevice& device) : device (device)
{
	namespace fs = std::filesystem;

	using namespace std::string_literals;

	fs::path shaders_dir ("assets");
	shaders_dir /= "shaders";

	std::vector<std::string> cmds;

	for (const auto& entry : fs::directory_iterator (shaders_dir))
	{
		auto filename = entry.path ().filename ();
		if (entry.is_regular_file ())
		{
			auto p = entry.path ();
			if (p.extension () == ".vert" || p.extension () == ".frag" || p.extension () == ".geom" ||
			    p.extension () == ".tesc" || p.extension () == ".tese" || p.extension () == ".comp")
			{
				fs::path in_path = entry.path ();
				fs::path out_path = entry.path ();
				out_path += ".spv";

				load_and_compile_module (in_path);
				Log.Debug (fmt::format (
				    "Compiled shader {}{}\n", in_path.stem ().string (), in_path.extension ().string ()));
			}
		}
	}
}



ShaderManager::~ShaderManager ()
{
	for (auto& [key, module] : module_map)
	{
		vkDestroyShaderModule (device.device, module.module, nullptr);
	}
}

std::optional<ShaderKey> ShaderManager::load_and_compile_module (std::filesystem::path file)
{
	auto file_data = compiler.load_file_data (file).value ();
	auto shader_type = GetShaderStage (file.extension ());
	auto spirv = compiler.compile_glsl_to_spriv (file.stem (), file_data, shader_type);

	if (!spirv.has_value ())
	{
		return {};
	}
	return create_module (file.stem ().string (), shader_type, spirv.value ());
}

std::optional<ShaderKey> ShaderManager::load_module (
    std::string const& name, std::string const& codePath, ShaderType type)
{
	std::vector<char> shaderCode;
	std::vector<uint32_t> codeAligned;

	auto pos_shaderCode = readShaderFile (codePath);
	if (!pos_shaderCode.has_value ())
	{
		Log.Error (fmt::format ("Shader at {} wont load, using defaults\n", codePath));
		return {};
	}

	shaderCode = pos_shaderCode.value ();

	codeAligned = std::vector<uint32_t> (shaderCode.size () / 4 + 1);
	memcpy (codeAligned.data (), shaderCode.data (), shaderCode.size ());


	return create_module (name, type, codeAligned);
}

std::optional<ShaderKey> ShaderManager::create_module (
    std::string const& name, ShaderType type, std::vector<uint32_t> const& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = code.data ();
	createInfo.codeSize = code.size () * 4;

	VkShaderModule vk_shaderModule;

	if (vkCreateShaderModule (device.device, &createInfo, nullptr, &vk_shaderModule) != VK_SUCCESS)
	{
		Log.Error (fmt::format ("Failed to create VkShaderModule for {}\n", name));
		return {};
	}

	ShaderKey key{ name, type };
	ShaderModule module{ type, vk_shaderModule };

	module_map[key] = module;

	return key;
}

void ShaderManager::delete_module (ShaderKey const& key)
{
	auto search = module_map.find (key);
	if (search != std::end (module_map))
	{
		module_map.erase (search);
	}
}

std::optional<ShaderModule> ShaderManager::get_module (ShaderKey const& key)
{
	auto mod = module_map.find (key);
	if (mod != module_map.end ())
	{
		return mod->second;
	}
	else
	{
		return {};
	}
}
std::optional<ShaderModule> ShaderManager::get_module (std::string const& name, ShaderType const type)
{
	return get_module ({ name, type });
}

std::optional<std::vector<char>> ShaderManager::readShaderFile (const std::string& filename)
{
	std::ifstream file (filename, std::ios::ate | std::ios::binary);

	if (!file.is_open ())
	{
		return {}; // don't throw, just return an optional
	}

	size_t fileSize = (size_t)file.tellg ();
	std::vector<char> buffer (fileSize);

	file.seekg (0);
	file.read (buffer.data (), fileSize);

	file.close ();

	return buffer;
}