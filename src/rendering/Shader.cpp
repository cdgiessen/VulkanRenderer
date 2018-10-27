#include "Shader.h"

#include <chrono>
#include <filesystem>
#include <thread>

#include <json.hpp>

#include "../core/CoreTools.h"
#include "../core/Logger.h"
#include "Initializers.h"
#include "RenderTools.h"

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

ShaderModule::ShaderModule (ShaderModuleType type, VkShaderModule module)
: type (type), module (module)
{
	switch (type)
	{
		case (ShaderModuleType::vertex):
			createInfo = initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_VERTEX_BIT, module);
			break;

		case (ShaderModuleType::fragment):
			createInfo = initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_FRAGMENT_BIT, module);
			break;

		case (ShaderModuleType::geometry):
			createInfo = initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_GEOMETRY_BIT, module);
			break;

		case (ShaderModuleType::tessEval):
			createInfo = initializers::pipelineShaderStageCreateInfo (
			    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, module);
			break;

		case (ShaderModuleType::tessControl):
			createInfo = initializers::pipelineShaderStageCreateInfo (
			    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, module);
			break;
	}
};

ShaderModuleSet::ShaderModuleSet (){};


ShaderModuleSet::ShaderModuleSet (ShaderModule vert,
    std::optional<ShaderModule> frag,
    std::optional<ShaderModule> geom,
    std::optional<ShaderModule> tessEval,
    std::optional<ShaderModule> tessControl)
:

  vert (vert),
  frag (frag),
  geom (geom),
  tessEval (tessEval),
  tessControl (tessControl)
{
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderModuleSet::ShaderStageCreateInfos ()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back (vert.createInfo);
	if (frag.has_value ()) shaderStages.push_back (frag->createInfo);
	if (geom.has_value ()) shaderStages.push_back (geom->createInfo);
	if (tessControl.has_value ()) shaderStages.push_back (tessControl->createInfo);
	if (tessEval.has_value ()) shaderStages.push_back (tessEval->createInfo);

	return shaderStages;
}

// time_t LastTimeWritten (std::filesystem::path entry)
//{
//	auto timeEntry = std::filesystem::last_write_time (entry);
//	time_t cftime = std::chrono::system_clock::to_time_t (timeEntry);
//	return cftime;
//}

void ReadInShaderDatabaseFile (std::string fileName)
{
	if (fileExists (fileName))
	{
		try
		{
			std::ifstream input (fileName);
			nlohmann::json j;
			input >> j;


			memory_dump = j["memory_dump_on_exit"];
			directionalLightCount = j["directional_light_count"];
			pointLightCount = j["point_light_count"];
			spotLightCount = j["spot_light_count"];
		}
		catch (std::runtime_error e)
		{
			Log::Debug << "Shader Database file was incorrect, creating a new one";
			SaveShaderDatabaseFile ();
		}
	}
	else
	{
		Log::Debug << "Shader Database doesn't exist, creating one";
		SaveShaderDatabaseFile ();
	}
}

void SaveShaderDatabaseFile ()
{
	nlohmann::json j;

	j["memory_dump_on_exit"] = memory_dump;

	j["directional_light_count"] = directionalLightCount;
	j["point_light_count"] = pointLightCount;
	j["spot_light_count"] = spotLightCount;

	std::ofstream outFile (fileName);
	outFile << std::setw (4) << j;
	outFile.close ();
}

void StartShaderCompilation (std::vector<std::string> strs)
{
	for (auto& str : strs)
		std::system (str.c_str ());
}

void CompileShaders (std::vector<std::string> filenames)
{
	unsigned int cts = std::thread::hardware_concurrency ();

	std::vector<std::thread> threads;
	for (int i = 0; i < cts; i++)
	{
		if (i == cts - 1)
		{
			auto cmds = std::vector<std::string> (std::begin (filenames) + i * filenames.size () / cts,
			    std::begin (filenames) + (i + 1) / cts);
			threads.push_back (std::thread (StartShaderCompilation, cmds));
		}
		else
		{
			auto cmds = std::vector<std::string> (
			    std::begin (filenames) + i * filenames.size () / cts, std::end (filenames));
			threads.push_back (std::thread (StartShaderCompilation, cmds));
		}
	}
	for (auto& t : threads)
	{
		t.join ();
	}
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

ShaderModule ShaderManager::loadShaderModule (const std::string& codePath, ShaderModuleType type)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	std::vector<char> shaderCode;

	auto pos_shaderCode = readShaderFile (codePath);
	if (!pos_shaderCode.has_value ())
	{
		Log::Debug << "Shader at " << codePath << " unable to load, using defaults instead\n";

		switch (type)
		{
			case (ShaderModuleType::vertex):
				createInfo.codeSize = sizeof (defaultVertexShader);
				createInfo.pCode = (uint32_t*)defaultVertexShader;

				break;
			case (ShaderModuleType::fragment):
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
