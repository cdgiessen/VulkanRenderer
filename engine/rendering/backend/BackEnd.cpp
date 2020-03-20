#include "BackEnd.h"

#include "core/Window.h"
#include "resources/Resource.h"

BackEnd::BackEnd (bool validationLayer, job::ThreadPool& thread_pool, Window& window, Resource::Resources& resource_man)
: device (window, validationLayer),
  vulkanSwapChain (device, window),
  async_task_queue (thread_pool, device),
  shaders (resource_man.shaders, device.device),
  pipeline_cache (device.device),
  models (resource_man.meshes, device, async_task_queue),
  textures (resource_man.textures, device, async_task_queue)
{
}