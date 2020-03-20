#include "Shader.h"

#include <filesystem>
#include <iomanip>

#include "StandAlone/DirStackFileIncluder.h"
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>

#include <nlohmann/json.hpp>

#include "core/JobSystem.h"
#include "core/Logger.h"
namespace Resource::Shader
{

const std::string shader_path = "assets/shaders/";
const std::string database_path = "assets/shader_db.json";

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
	/* .MaxSamples = */ 4,
	/* .maxMeshOutputVerticesNV = */ 0,
	/* .maxMeshOutputPrimitivesNV = */ 0,
	/* .maxMeshWorkGroupSizeX_NV = */ 0,
	/* .maxMeshWorkGroupSizeY_NV = */ 0,
	/* .maxMeshWorkGroupSizeZ_NV = */ 0,
	/* .maxTaskWorkGroupSizeX_NV = */ 0,
	/* .maxTaskWorkGroupSizeY_NV = */ 0,
	/* .maxTaskWorkGroupSizeZ_NV = */ 0,
	/* .maxMeshViewCountNV = */ 0,

	DefaultTBuiltInLimits };

ShaderType GetShaderStage (const std::string& stage)
{
	if (stage == "vert" || stage == "vertex" || stage == ".vert")
	{
		return ShaderType::vertex;
	}
	else if (stage == "frag" || stage == "fragment" || stage == ".frag")
	{
		return ShaderType::fragment;
	}
	else if (stage == "tesc" || stage == "tess_control" || stage == ".tesc")
	{
		return ShaderType::tess_control;
	}
	else if (stage == "tese" || stage == "tess_eval" || stage == ".tese")
	{
		return ShaderType::tess_eval;
	}
	else if (stage == "geom" || stage == "geometry" || stage == ".geom")
	{
		return ShaderType::geometry;
	}
	else if (stage == "comp" || stage == "compute" || stage == ".comp")
	{
		return ShaderType::compute;
	}
	Log.Error (fmt::format ("Found error shader type of {}", stage));
	return ShaderType::error;
}

std::optional<std::vector<char>> readShaderFile (const std::string& filename)
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

// time_t LastTimeWritten (std::filesystem::path entry)
//{
//	auto timeEntry = std::filesystem::last_write_time (entry);
//	return std::chrono::system_clock::to_time_t (timeEntry);
//}

void to_json (nlohmann::json& j, const ShaderDatabase::DBHandle& handle)
{
	j = nlohmann::json (
	    { { "name", handle.name }, { "type", static_cast<int> (handle.type) }, { "id", handle.id } });
}

void from_json (const nlohmann::json& j, ShaderDatabase::DBHandle& handle)
{
	j.at ("name").get_to (handle.name);
	std::string type;
	j.at ("type").get_to (type);
	handle.type = GetShaderStage (type);
	j.at ("id").get_to (handle.id);
}

ShaderDatabase::ShaderDatabase ()
{
	Load ();
	Refresh ();
}
void ShaderDatabase::Load ()
{
	if (std::filesystem::exists (database_path))
	{
		try
		{
			std::ifstream input (database_path);
			nlohmann::json j;

			input >> j;

			for (auto& entry : j)
			{
				entries.push_back (entry);
			}
		}
		catch (nlohmann::detail::parse_error& e)
		{
			Log.Debug (fmt::format ("Shader Database file was bas, creating a new one: {}", e.what ()));
			Save ();
		}
		catch (std::runtime_error& e)
		{
			Log.Debug (fmt::format ("Shader Database file was incorrect, creating a new one: {}", e.what ()));
			Save ();
		}
	}
	else
	{
		Log.Debug ("Shader Database doesn't exist, creating one");
		Save ();
	}
}

void ShaderDatabase::Save ()
{
	nlohmann::json j;

	for (auto& entry : entries)
	{
		j[entry.name] = entry;
	}

	std::ofstream outFile (database_path);
	outFile << std::setw (4) << j;
	outFile.close ();
}

void ShaderDatabase::Refresh () {}


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
	if (!is_setup) // TODO: do proper atomic guard here
	{
		is_setup = true;
		glslang::InitializeProcess ();
	}
}

// Load GLSL into a string
std::optional<std::string> ShaderCompiler::LoadFileData (const std::string& filename)
{
	std::ifstream file (filename);

	if (!file.is_open ())
	{
		Log.Error (fmt::format ("Failed to load shader: {}", filename));
		return {};
	}

	return std::string ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
}

std::optional<std::vector<uint32_t>> const ShaderCompiler::compile_glsl_to_spirv (std::string const& shader_name,
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
		Log.Error (fmt::format ("GLSL Preprocessing Failed for: {}", shader_name));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	const char* PreprocessedCStr = PreprocessedGLSL.c_str ();
	Shader.setStrings (&PreprocessedCStr, 1);

	if (!Shader.parse (&Resources, 100, false, messages))
	{
		Log.Error (fmt::format ("GLSL Parsing Failed for: {}", shader_name));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	glslang::TProgram Program;
	Program.addShader (&Shader);

	if (!Program.link (messages))
	{
		Log.Error (fmt::format ("GLSL Linking Failed for: {}", shader_name));
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

Shaders::Shaders (job::ThreadPool& thread_pool) : thread_pool (thread_pool)
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
				AddShader (in_path.filename ().string (), in_path.string ());
				Log.Debug (fmt::format (
				    "Compiled shader {}.{}", in_path.stem ().string (), in_path.extension ().string ()));
			}
		}
	}
}

std::vector<uint32_t> AlignData (std::vector<char> const& code)
{
	std::vector<uint32_t> codeAligned;

	codeAligned = std::vector<uint32_t> (code.size () / 4 + 1);
	memcpy (codeAligned.data (), code.data (), code.size ());

	return codeAligned;
}

ShaderID Shaders::AddShader (std::string name, std::string path)
{

	auto shader_chars = compiler.LoadFileData (path);
	if (!shader_chars.has_value ())
	{
		Log.Error (fmt::format ("Couldn't find shader {}", name));
	}
	std::filesystem::path p = path;
	ShaderType type;
	if (p.has_extension ())
		type = GetShaderStage (p.extension ().string ());
	else
		type = GetShaderStage (name);

	auto spirv_data =
	    compiler.compile_glsl_to_spirv (path, shader_chars.value (), type, "assets/shaders/common");

	if (spirv_data.has_value ())
	{
		std::lock_guard lg (lock);
		while (shaders.count (cur_id) == 1)
			cur_id++;
		ShaderID new_id = cur_id;
		ShaderInfo info = ShaderInfo{ name, type, std::move (spirv_data.value ()) };
		shaders.emplace (new_id, std::move (info));
		return new_id;
	}
	return -1;
}

std::vector<uint32_t> Shaders::GetSpirVData (ShaderID id)
{
	std::lock_guard lg (lock);
	return shaders.at (id).spirv_data;
}
std::vector<uint32_t> Shaders::GetSpirVData (std::string const& name, ShaderType type)
{
	std::lock_guard lg (lock);
	for (auto& [key, info] : shaders)
	{
		if (info.name == name && info.type == type) return info.spirv_data;
	}
	return {};
}



} // namespace Resource::Shader