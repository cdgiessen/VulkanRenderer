#include "Shader.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <utility>

#include "resources/Shader.h"

#include "core/JobSystem.h"
#include "core/Logger.h"

#include "Device.h"
#include "Initializers.h"
#include "RenderTools.h"

#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"

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
			break;
	}
	Log.Error ("Shader module type not correct");
	return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_VERTEX_BIT, module);
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


ShaderManager::ShaderManager (Resource::Shader::Manager& resource_shader_manager, VulkanDevice& device)
: resource_shader_manager (resource_shader_manager), device (device)
{
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
	auto file_data = compiler.load_file_data (file.string ()).value ();
	auto shader_type = GetShaderStage (file.extension ().string ());
	auto spirv = compiler.compile_glsl_to_spriv (
	    file.stem ().string (), file_data, shader_type, file.parent_path () / "common");

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