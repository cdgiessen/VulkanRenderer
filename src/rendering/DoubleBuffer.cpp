#include "DoubleBuffer.h"

#include "Buffer.h"
#include "Device.h"
#include "Renderer.h"

DoubleBuffer::DoubleBuffer (VulkanDevice& device, BufCreateDetails const& create_details)
//: buffers ({ { device, create_details }, { device, create_details } })
{
	buffers[0] = VulkanBuffer (device, create_details);
	buffers[1] = VulkanBuffer (device, create_details);
}

VulkanBuffer const& DoubleBuffer::Read () { return buffers[cur_read]; }
VulkanBuffer& DoubleBuffer::Write () { return buffers[cur_write]; }

void DoubleBuffer::Advance ()
{
	cur_read = cur_write;
	cur_write = (cur_write + 1) % 2;
}

// constexpr int MaxTransformCount = 16384;

// GPU_DoubleBuffer::GPU_DoubleBuffer (VulkanDevice& device, DescriptorManager& descriptor_man,
// RenderSettings& settings) : device (device), descriptor_man (descriptor_man)
// {
// 	for (auto& data : d_buffers)
// 	{
// 		auto globalDetails = uniform_details (sizeof (GlobalData));
// 		globalDetails.persistentlyMapped = true;

// 		auto cameraDetails = uniform_details (sizeof (CameraData) * settings.cameraCount);
// 		auto dirLightDetails = uniform_details (sizeof (DirectionalLight) * settings.directionalLightCount);
// 		auto pointLightDetails = uniform_details (sizeof (PointLight) * settings.pointLightCount);
// 		auto spotLightDetails = uniform_details (sizeof (SpotLight) * settings.spotLightCount);

// 		data.globalVariableBuffer = std::make_unique<VulkanBuffer> (device, globalDetails);

// 		data.cameraDataBuffer = std::make_unique<VulkanBuffer> (device, cameraDetails);
// 		data.sunBuffer = std::make_unique<VulkanBuffer> (device, dirLightDetails);
// 		data.pointLightsBuffer = std::make_unique<VulkanBuffer> (device, pointLightDetails);
// 		data.spotLightsBuffer = std::make_unique<VulkanBuffer> (device, spotLightDetails);


// 		auto transformDetails = uniform_dynamic_details (MaxTransformCount * sizeof (TransformMatrixData));
// 		data.dynamicTransformBuffer = std::make_unique<VulkanBuffer> (device, transformDetails);


// 		// Frame data

// 		std::vector<DescriptorSetLayoutBinding> m_bindings = {
// 			{ DescriptorType::uniform_buffer, ShaderStage::all_graphics, 0, 1 },
// 			{ DescriptorType::uniform_buffer, ShaderStage::all_graphics, 1, 1 }
// 		};

// 		auto frameDatalayout = descriptor_man.CreateDescriptorSetLayout (DescriptorLayout
// (m_bindings)); 		frameDataDescriptorSet = descriptor_man.CreateDescriptorSet (frameDatalayout);

// 		std::vector<DescriptorUse> writes;
// 		writes.push_back (DescriptorUse (0, 1, data.globalVariableBuffer->GetResource ()));
// 		writes.push_back (DescriptorUse (1, 1, data.cameraDataBuffer->GetResource ()));
// 		frameDataDescriptorSet.Update (device.device, writes);

// 		auto desLayout = descriptor_man.GetLayout (frameDatalayout);
// 		auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo (&desLayout, 1);

// 		if (vkCreatePipelineLayout (device.device, &pipelineLayoutInfo, nullptr, &frameDataDescriptorLayout) != VK_SUCCESS)
// 		{
// 			throw std::runtime_error ("failed to create pipeline layout!");
// 		}

// 		// Lighting
// 		std::vector<DescriptorSetLayoutBinding> l_bindings = {
// 			{ DescriptorType::uniform_buffer, ShaderStage::fragment, 0, 1 },
// 			{ DescriptorType::uniform_buffer, ShaderStage::fragment, 1, 1 },
// 			{ DescriptorType::uniform_buffer, ShaderStage::fragment, 2, 1 }
// 		};
// 		auto lightingLayout = descriptor_man.CreateDescriptorSetLayout (DescriptorLayout
// (l_bindings)); 		lightingDescriptorSet = descriptor_man.CreateDescriptorSet (lightingLayout);

// 		std::vector<DescriptorUse> writes;
// 		writes.push_back (DescriptorUse (0, 1, data.sunBuffer->GetResource ()));
// 		writes.push_back (DescriptorUse (1, 1, data.pointLightsBuffer->GetResource ()));
// 		writes.push_back (DescriptorUse (2, 1, data.spotLightsBuffer->GetResource ()));
// 		lightingDescriptorSet.Update (device.device, writes);

// 		std::vector<VkDescriptorSetLayout> layouts;
// 		layouts.push_back (descriptor_man.GetLayout (frameDatalayout));
// 		layouts.push_back (descriptor_man.GetLayout (lightingLayout));

// 		auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo (layouts.data (), 2);

// 		if (vkCreatePipelineLayout (device.device, &pipelineLayoutInfo, nullptr, &lightingDescriptorLayout) != VK_SUCCESS)
// 		{
// 			throw std::runtime_error ("failed to create pipeline layout!");
// 		}
// 	}
// // Transformation Matrices -- Dynamic Uniform Buffer
// {

// 	dynamicTransformDescriptor = std::make_unique<VulkanDescriptor> (device);
// 	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
// 	m_bindings.push_back (VulkanDescriptor::CreateBinding (
// 	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
// 	dynamicTransformDescriptor->SetupLayout (m_bindings);

// 	std::vector<DescriptorPoolSize> poolSizes;
// 	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2));
// 	dynamicTransformDescriptor->SetupPool (poolSizes, 2);
// 	for (auto& data : d_buffers)
// 	{
// 		data.dynamicTransformDescriptorSet = dynamicTransformDescriptor->CreateDescriptorSet ();

// 		std::vector<DescriptorUse> writes;
// 		writes.push_back (DescriptorUse (0, 1, data.dynamicTransformBuffer->GetResource ()));
// 		dynamicTransformDescriptor->UpdateDescriptorSet (data.dynamicTransformDescriptorSet, writes);
// 	}
// }
// }

// GPU_DoubleBuffer::~GPU_DoubleBuffer ()
// {
// 	vkDestroyPipelineLayout (device.device, frameDataDescriptorLayout, nullptr);
// 	vkDestroyPipelineLayout (device.device, lightingDescriptorLayout, nullptr);
// }

// void GPU_DoubleBuffer::Update (GlobalData& globalData,
//     std::vector<CameraData>& cameraData,
//     std::vector<DirectionalLight>& directionalLights,
//     std::vector<PointLight>& pointLights,
//     std::vector<SpotLight>& spotLights)
// {
// 	d_buffers.at (cur_index).globalVariableBuffer->CopyToBuffer (globalData);
// 	d_buffers.at (cur_index).cameraDataBuffer->CopyToBuffer (cameraData);
// 	d_buffers.at (cur_index).sunBuffer->CopyToBuffer (directionalLights);
// 	d_buffers.at (cur_index).pointLightsBuffer->CopyToBuffer (pointLights);
// 	d_buffers.at (cur_index).spotLightsBuffer->CopyToBuffer (spotLights);
// }


// int GPU_DoubleBuffer::CurIndex () { return cur_index; }

// void GPU_DoubleBuffer::AdvanceFrameCounter ()
// {
// 	cur_index = (cur_index + 1) % 2; // alternate between 0 & 1
// }

// void VulkanRenderer::AddGlobalLayouts (std::vector<VkDescriptorSetLayout>& layouts)
// {
// 	layouts.push_back (frameDataDescriptor->GetLayout ());
// 	layouts.push_back (lightingDescriptor->GetLayout ());
// }

// std::vector<VkDescriptorSetLayout> VulkanRenderer::GetGlobalLayouts ()
// {
// 	return { frameDataDescriptor->GetLayout (), lightingDescriptor->GetLayout () };
// }

// void GPU_DoubleBuffer::BindFrameDataDescriptorSet (int index, VkCommandBuffer cmdBuf)
// {
// 	d_buffers.at (index).frameDataDescriptorSet.Bind (cmdBuf, frameDataDescriptorLayout, 0);
// }
// void GPU_DoubleBuffer::BindLightingDataDescriptorSet (int index, VkCommandBuffer cmdBuf)
// {
// 	d_buffers.at (index).lightingDescriptorSet.Bind (cmdBuf, lightingDescriptorLayout, 1);
// }
