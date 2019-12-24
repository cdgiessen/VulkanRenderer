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
#include "RenderTools.h"
#include "rendering/Initializers.h"

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
		// printf ("Image %s at set = %u, binding = %u", resource.name.c_str (), set, binding);

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

ShaderModule::ShaderModule (VkDevice device, ShaderType type, std::vector<uint32_t> const& code)
: device (device), type (type)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = code.data ();
	createInfo.codeSize = code.size () * 4;

	if (vkCreateShaderModule (device, &createInfo, nullptr, &module) != VK_SUCCESS)
	{
		Log.Error (fmt::format ("Failed to create VkShaderModule for"));
	}
}

ShaderModule::~ShaderModule () {}

ShaderModule::ShaderModule (ShaderModule&& mod)
: device (mod.device), type (mod.type), module (mod.module)
{
	mod.module = nullptr;
}
ShaderModule& ShaderModule::operator= (ShaderModule&& mod)
{
	device = mod.device;
	type = mod.type;
	module = mod.module;
	mod.module = nullptr;
	return *this;
}

VkPipelineShaderStageCreateInfo GetCreateInfo (ShaderType type, VkShaderModule module)
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
		default:
			Log.Error ("Shader module type not correct");
			return initializers::pipelineShaderStageCreateInfo (VK_SHADER_STAGE_VERTEX_BIT, module);
	}
};

ShaderModuleSet::ShaderModuleSet (){};
ShaderModuleSet::ShaderModuleSet (VkShaderModule vert, VkShaderModule frag)
: vert (vert), frag (frag)
{
}

ShaderModuleSet& ShaderModuleSet::Vertex (VkShaderModule vert)
{
	this->vert = vert;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::Fragment (VkShaderModule frag)
{
	this->frag = frag;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::Geometry (VkShaderModule geom)
{
	this->geom = geom;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::TessControl (VkShaderModule tess_control)
{
	this->tess_control = tess_control;
	return *this;
}
ShaderModuleSet& ShaderModuleSet::TessEval (VkShaderModule tess_eval)
{
	this->tess_eval = tess_eval;
	return *this;
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderModuleSet::ShaderStageCreateInfos ()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	if (vert != VK_NULL_HANDLE) shaderStages.push_back (GetCreateInfo (ShaderType::vertex, vert));
	if (frag != VK_NULL_HANDLE) shaderStages.push_back (GetCreateInfo (ShaderType::fragment, frag));
	if (geom != VK_NULL_HANDLE) shaderStages.push_back (GetCreateInfo (ShaderType::geometry, geom));
	if (tess_control != VK_NULL_HANDLE)
		shaderStages.push_back (GetCreateInfo (ShaderType::tessControl, tess_control));
	if (tess_eval != VK_NULL_HANDLE)
		shaderStages.push_back (GetCreateInfo (ShaderType::tessEval, tess_eval));

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

ShaderID ShaderManager::CreateModule (std::string const& name, ShaderType type, std::vector<uint32_t> const& code)
{
	ShaderModule module (device.device, type, code);

	ShaderID new_id = cur_id++;
	module_map.emplace (new_id, std::move (module));

	return new_id;
}

void ShaderManager::DeleteModule (ShaderID const& id)
{
	auto search = module_map.find (id);
	if (search != std::end (module_map))
	{
		module_map.erase (search);
	}
}

VkShaderModule ShaderManager::GetModule (ShaderID const& id)
{
	auto mod = module_map.find (id);
	if (mod != module_map.end ())
	{
		return mod->second.module;
	}
	else
	{
		return VK_NULL_HANDLE;
	}
}
VkShaderModule ShaderManager::GetModule (std::string name, ShaderType type)
{
	auto spirv_data =
	    resource_shader_manager.GetSpirVData (name, static_cast<Resource::Shader::ShaderType> (type));
	ShaderID id = CreateModule (name, type, spirv_data);
	return GetModule (id);
}