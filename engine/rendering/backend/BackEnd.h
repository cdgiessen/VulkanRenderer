#pragma once

#include "AsyncTask.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Device.h"
#include "FrameResources.h"
#include "Model.h"
#include "Pipeline.h"
#include "RenderStructs.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "Wrappers.h"

class Window;

namespace Resource
{
class ResourceManager;
}


struct BackEnd
{
	BackEnd (bool validationLayer, job::TaskManager& task_manager, Window& window, Resource::ResourceManager& resource_man);

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	AsyncTaskManager async_task_manager;
	DescriptorManager descriptor_manager;
	ShaderManager shader_manager;
	PipelineManager pipeline_manager;
	ModelManager model_manager;
	TextureManager texture_manager;
};