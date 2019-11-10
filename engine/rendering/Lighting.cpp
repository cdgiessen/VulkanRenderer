#include "Lighting.h"

#include "Buffer.h"
#include "Descriptor.h"
#include "Device.h"
LightingManager::LightingManager (VulkanDevice& device)
: device (device),
  directional_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (DirectionalLight))),
  point_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (PointLight))),
  spot_gpu_data (device, uniform_array_details (MaxDirectionalLightCount, sizeof (SpotLight)))
{
}