#include "Shader.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

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
#include <utility>

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

VkShaderModule loadShaderModule (VkDevice device, const std::string& codePath)
{
	auto shaderCode = readFile (codePath);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size ();

	std::vector<uint32_t> codeAligned (shaderCode.size () / 4 + 1);
	memcpy (codeAligned.data (), shaderCode.data (), shaderCode.size ());

	createInfo.pCode = codeAligned.data ();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule (device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create shader module!");
	}

	return shaderModule;
}
// clang-format off
static uint32_t defaultVertexShader[] = { 0x07230203,
	0x07230203, 0x00010000, 0x00080002, 0x0000002a, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000a, 0x00000020, 0x00030003, 0x00000002, 0x000001c2, 0x00090004, 0x415f4c47, 0x735f4252, 0x72617065, 0x5f657461, 0x64616873,
	0x6f5f7265, 0x63656a62, 0x00007374, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x00000008, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000008, 0x00000000,
	0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000000a, 0x00000000, 0x00070005, 0x0000000f, 0x656d6143, 0x6e556172, 0x726f6669, 0x6675426d, 0x00726566, 0x00050006, 0x0000000f, 0x00000000,
	0x77656976, 0x00000000, 0x00050006, 0x0000000f, 0x00000001, 0x6a6f7270, 0x00000000, 0x00060006, 0x0000000f, 0x00000002, 0x656d6163, 0x6f506172, 0x00000073, 0x00050006, 0x0000000f, 0x00000003,
	0x656d6974, 0x00000000, 0x00030005, 0x00000011, 0x006f6263, 0x00070005, 0x00000019, 0x66696e55, 0x426d726f, 0x65666675, 0x6a624f72, 0x00746365, 0x00050006, 0x00000019, 0x00000000, 0x65646f6d,
	0x0000006c, 0x00050006, 0x00000019, 0x00000001, 0x6d726f6e, 0x00006c61, 0x00030005, 0x0000001b, 0x006f6275, 0x00050005, 0x00000020, 0x6f506e69, 0x69746973, 0x00006e6f, 0x00050048, 0x00000008,
	0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x00000008, 0x00000002, 0x00040048, 0x0000000f, 0x00000000, 0x00000005, 0x00050048, 0x0000000f, 0x00000000, 0x00000023, 0x00000000, 0x00050048,
	0x0000000f, 0x00000000, 0x00000007, 0x00000010, 0x00040048, 0x0000000f, 0x00000001, 0x00000005, 0x00050048, 0x0000000f, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x0000000f, 0x00000001,
	0x00000007, 0x00000010, 0x00050048, 0x0000000f, 0x00000002, 0x00000023, 0x00000080, 0x00050048, 0x0000000f, 0x00000003, 0x00000023, 0x0000008c, 0x00030047, 0x0000000f, 0x00000002, 0x00040047,
	0x00000011, 0x00000022, 0x00000000, 0x00040047, 0x00000011, 0x00000021, 0x00000000, 0x00040048, 0x00000019, 0x00000000, 0x00000005, 0x00050048, 0x00000019, 0x00000000, 0x00000023, 0x00000000,
	0x00050048, 0x00000019, 0x00000000, 0x00000007, 0x00000010, 0x00040048, 0x00000019, 0x00000001, 0x00000005, 0x00050048, 0x00000019, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x00000019,
	0x00000001, 0x00000007, 0x00000010, 0x00030047, 0x00000019, 0x00000002, 0x00040047, 0x0000001b, 0x00000022, 0x00000000, 0x00040047, 0x0000001b, 0x00000021, 0x00000001, 0x00040047, 0x00000020,
	0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x0003001e, 0x00000008,
	0x00000007, 0x00040020, 0x00000009, 0x00000003, 0x00000008, 0x0004003b, 0x00000009, 0x0000000a, 0x00000003, 0x00040015, 0x0000000b, 0x00000020, 0x00000001, 0x0004002b, 0x0000000b, 0x0000000c,
	0x00000000, 0x00040018, 0x0000000d, 0x00000007, 0x00000004, 0x00040017, 0x0000000e, 0x00000006, 0x00000003, 0x0006001e, 0x0000000f, 0x0000000d, 0x0000000d, 0x0000000e, 0x00000006, 0x00040020,
	0x00000010, 0x00000002, 0x0000000f, 0x0004003b, 0x00000010, 0x00000011, 0x00000002, 0x0004002b, 0x0000000b, 0x00000012, 0x00000001, 0x00040020, 0x00000013, 0x00000002, 0x0000000d, 0x0004001e,
	0x00000019, 0x0000000d, 0x0000000d, 0x00040020, 0x0000001a, 0x00000002, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000002, 0x00040020, 0x0000001f, 0x00000001, 0x0000000e, 0x0004003b,
	0x0000001f, 0x00000020, 0x00000001, 0x0004002b, 0x00000006, 0x00000022, 0x3f800000, 0x00040020, 0x00000028, 0x00000003, 0x00000007, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003,
	0x000200f8, 0x00000005, 0x00050041, 0x00000013, 0x00000014, 0x00000011, 0x00000012, 0x0004003d, 0x0000000d, 0x00000015, 0x00000014, 0x00050041, 0x00000013, 0x00000016, 0x00000011, 0x0000000c,
	0x0004003d, 0x0000000d, 0x00000017, 0x00000016, 0x00050092, 0x0000000d, 0x00000018, 0x00000015, 0x00000017, 0x00050041, 0x00000013, 0x0000001c, 0x0000001b, 0x0000000c, 0x0004003d, 0x0000000d,
	0x0000001d, 0x0000001c, 0x00050092, 0x0000000d, 0x0000001e, 0x00000018, 0x0000001d, 0x0004003d, 0x0000000e, 0x00000021, 0x00000020, 0x00050051, 0x00000006, 0x00000023, 0x00000021, 0x00000000,
	0x00050051, 0x00000006, 0x00000024, 0x00000021, 0x00000001, 0x00050051, 0x00000006, 0x00000025, 0x00000021, 0x00000002, 0x00070050, 0x00000007, 0x00000026, 0x00000023, 0x00000024, 0x00000025,
	0x00000022, 0x00050091, 0x00000007, 0x00000027, 0x0000001e, 0x00000026, 0x00050041, 0x00000028, 0x00000029, 0x0000000a, 0x0000000c, 0x0003003e, 0x00000029, 0x00000027, 0x000100fd, 0x00010038};

static uint32_t defaultFragmentShader[] = { 0x07230203,
	0x07230203, 0x00010000, 0x00080002, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0006000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00090004, 0x415f4c47, 0x735f4252, 0x72617065,
	0x5f657461, 0x64616873, 0x6f5f7265, 0x63656a62, 0x00007374, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040047, 0x00000009,
	0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008,
	0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x0004002b, 0x00000006, 0x0000000a, 0x3f800000, 0x0004002b, 0x00000006, 0x0000000b, 0x00000000, 0x0007002c, 0x00000007,
	0x0000000c, 0x0000000a, 0x0000000b, 0x0000000a, 0x0000000a, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0003003e, 0x00000009, 0x0000000c, 0x000100fd,
	0x00010038 };
// clang-format on

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



void StartShaderCompilation (std::vector<std::string> strs)
{
	for (auto& str : strs)
	{
		int ret = std::system (str.c_str ());
		if (ret != 0) Log.Error (fmt::format ("Failed to compile {}", str));
	}
}

// std::vector<uint32_t> CompileShaderToSpivModule (std::string filename) { return {}; }


void CompileShaders (std::vector<std::string> filenames)
{
	unsigned int cts = std::thread::hardware_concurrency ();

	std::vector<std::vector<std::string>> buckets (cts);
	for (size_t i = 0; i < filenames.size (); i++)
	{
		int index = i % cts;
		buckets.at (index).push_back (filenames.at (i));
	}

	auto signal = std::make_shared<job::TaskSignal> ();

	std::vector<job::Task> tasks;
	for (auto& bucket : buckets)
	{
		tasks.push_back (job::Task (signal, [&]() { StartShaderCompilation (bucket); }));
	}
	taskManager.Submit (tasks, job::TaskType::currentFrame);

	signal->Wait ();
}

ShaderManager::ShaderManager (VulkanDevice& device) : device (device)
{
	shaderModules.reserve (32);

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
				std::string cmd;
#ifdef WIN32
				cmd = "glslangvalidator.exe -V "s + in_path.generic_string () + " -o "s +
				      out_path.generic_string ();
#endif // WIN32
#ifdef __linux__
				cmd = "./glslangValidator -V "s + in_path.generic_string () + " -o "s +
				      out_path.generic_string ();
#endif // __linux__
				cmds.push_back (cmd);
			}
		}
	}

	CompileShaders (cmds);
}

ShaderManager::~ShaderManager ()
{

	for (auto& module : shaderModules)
	{
		vkDestroyShaderModule (device.device, module.module, nullptr);
	}
}

ShaderModule ShaderManager::loadShaderModule (const std::string& codePath, ShaderType type)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	std::vector<char> shaderCode;

	auto pos_shaderCode = readShaderFile (codePath);
	if (!pos_shaderCode.has_value ())
	{
		Log.Error (fmt::format ("Shader at {} wont load, using defaults\n", codePath));

		switch (type)
		{
			case (ShaderType::vertex):
				createInfo.codeSize = sizeof (defaultVertexShader);
				createInfo.pCode = (uint32_t*)defaultVertexShader;

				break;
			case (ShaderType::fragment):
				createInfo.codeSize = sizeof (defaultFragmentShader);
				createInfo.pCode = (uint32_t*)defaultFragmentShader;

				break;
			default:
				throw std::runtime_error (
				    "shader type does not exist! (no default geometry or tess shaders");
		}
	}
	else
	{
		shaderCode = pos_shaderCode.value ();

		createInfo.codeSize = shaderCode.size ();

		std::vector<uint32_t> codeAligned (shaderCode.size () / 4 + 1);
		memcpy (codeAligned.data (), shaderCode.data (), shaderCode.size ());

		createInfo.pCode = reinterpret_cast<const uint32_t*> (shaderCode.data ());
	}

	VkShaderModule vk_shaderModule;

	if (vkCreateShaderModule (device.device, &createInfo, nullptr, &vk_shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to create shader module!");
	}

	ShaderModule module{ type, vk_shaderModule };

	shaderModules.push_back (module);
	return module;
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

static std::atomic_bool is_setup = false;

ShaderCompiler::ShaderCompiler ()
{
	if (!is_setup)
	{
		is_setup = true;
		glslang::InitializeProcess ();
	}
}

std::string GetFilePath (const std::string& str)
{
	size_t found = str.find_last_of ("/\\");
	return str.substr (0, found);
	// size_t FileName = str.substr(found+1);
}

std::string GetSuffix (const std::string& name)
{
	const size_t pos = name.rfind ('.');
	return (pos == std::string::npos) ? "" : name.substr (name.rfind ('.') + 1);
}

const std::vector<uint32_t> ShaderCompiler::LoadAndCompileShader (const std::string& filename)
{
	auto shader_string = load_file (filename).value ();
	auto shader_type = GetShaderStage (GetSuffix (filename));
	return CompileShaderString (filename, shader_string, shader_type).value ();
}


// Load GLSL into a string
std::optional<std::string> ShaderCompiler::load_file (const std::string& filename)
{
	std::ifstream file (filename);

	if (!file.is_open ())
	{
		Log.Error (fmt::format ("Failed to load shader: {}", filename));
		return {};

		// throw std::runtime_error ("failed to open file: " + filename);
	}

	return std::string ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
}

std::optional<std::vector<unsigned int>> const ShaderCompiler::CompileShaderString (
    std::string const& shader_filename, std::string const& shader_string, ShaderType const shader_type)
{
	const char* InputCString = shader_string.c_str ();

	// EShLanguage ShaderType = GetShaderStage (GetSuffix (filename));
	glslang::TShader Shader (static_cast<EShLanguage> (shader_type));

	Shader.setStrings (&InputCString, 1);

	int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
	glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

	Shader.setEnvInput (
	    glslang::EShSourceGlsl, static_cast<EShLanguage> (shader_type), glslang::EShClientVulkan, ClientInputSemanticsVersion);
	Shader.setEnvClient (glslang::EShClientVulkan, VulkanClientVersion);
	Shader.setEnvTarget (glslang::EShTargetSpv, TargetVersion);

	const TBuiltInResource DefaultTBuiltInResource = {};

	TBuiltInResource Resources;
	Resources = DefaultTBuiltInResource;
	EShMessages messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);

	const int DefaultVersion = 100;

	DirStackFileIncluder Includer;

	// Get Path of File
	std::string Path = GetFilePath (shader_filename);
	Includer.pushExternalLocalDirectory (Path);

	std::string PreprocessedGLSL;

	if (!Shader.preprocess (&Resources, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer))
	{
		Log.Error (fmt::format ("GLSL Preprocessing Failed for: {}", shader_filename));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	const char* PreprocessedCStr = PreprocessedGLSL.c_str ();
	Shader.setStrings (&PreprocessedCStr, 1);

	if (!Shader.parse (&Resources, 100, false, messages))
	{
		Log.Error (fmt::format ("GLSL Parsing Failed for: {}", shader_filename));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	glslang::TProgram Program;
	Program.addShader (&Shader);

	if (!Program.link (messages))
	{
		Log.Error (fmt::format ("GLSL Linking Failed for: {}", shader_filename));
		Log.Error (Shader.getInfoLog ());
		Log.Error (Shader.getInfoDebugLog ());
		return {};
	}

	std::vector<unsigned int> SpirV;
	spv::SpvBuildLogger logger;
	glslang::SpvOptions spvOptions;
	glslang::GlslangToSpv (
	    *Program.getIntermediate (static_cast<EShLanguage> (shader_type)), SpirV, &logger, &spvOptions);

	return SpirV;
}