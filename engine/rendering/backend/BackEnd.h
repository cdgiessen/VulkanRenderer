#pragma once

#include "AsyncTask.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Device.h"
#include "FrameResources.h"
#include "Model.h"
#include "Pipeline.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "Wrappers.h"

class Window;

namespace Resource
{
class Resources;
}


struct BackEnd
{
	BackEnd (bool validationLayer, job::ThreadPool& thread_pool, Window& window, Resource::Resources& resource_man);

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	AsyncTaskQueue async_task_queue;
	Shaders shaders;
	PipelineCache pipeline_cache;
	Models models;
	Textures textures;
};