#include "BackEnd.h"

#include "core/Window.h"
#include "resources/Resource.h"

BackEnd::BackEnd (bool validationLayer, job::TaskManager& task_manager, Window& window, Resource::ResourceManager& resource_man)
: device (window, validationLayer),
  vulkanSwapChain (device, window),
  async_task_manager (task_manager, device),
  descriptor_manager (device),
  shader_manager (resource_man.shader_manager, device),
  pipeline_manager (device, async_task_manager),
  model_manager (resource_man.mesh_manager, device, async_task_manager),
  texture_manager (resource_man.texture_manager, device, async_task_manager)
{
}