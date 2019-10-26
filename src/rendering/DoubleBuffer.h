#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Descriptor.h"
#include "RenderStructs.h"

class VulkanDevice;
class VulkanBuffer;
class RenderSettings;

class GPU_DoubleBuffer
{
	public:
	GPU_DoubleBuffer (VulkanDevice& device, DescriptorManager& descriptor_man, RenderSettings& settings);
	GPU_DoubleBuffer (GPU_DoubleBuffer const& buf) = delete;
	GPU_DoubleBuffer& operator= (GPU_DoubleBuffer const& buf) = delete;

	~GPU_DoubleBuffer ();

	void Update (GlobalData& globalData,
	    std::vector<CameraData>& cameraData,
	    std::vector<DirectionalLight>& directionalLights,
	    std::vector<PointLight>& pointLights,
	    std::vector<SpotLight>& spotLights);

	int CurIndex ();
	void AdvanceFrameCounter ();

	void AddGlobalLayouts (std::vector<VkDescriptorSetLayout>& layouts);
	std::vector<VkDescriptorSetLayout> GetGlobalLayouts ();

	void BindFrameDataDescriptorSet (int index, VkCommandBuffer cmdBuf);
	void BindLightingDataDescriptorSet (int index, VkCommandBuffer cmdBuf);

	private:
	struct Dynamic
	{
		std::unique_ptr<VulkanBuffer> globalVariableBuffer;
		std::unique_ptr<VulkanBuffer> cameraDataBuffer;
		std::unique_ptr<VulkanBuffer> sunBuffer;
		std::unique_ptr<VulkanBuffer> pointLightsBuffer;
		std::unique_ptr<VulkanBuffer> spotLightsBuffer;

		std::unique_ptr<VulkanBuffer> dynamicTransformBuffer;

		DescriptorSet frameDataDescriptorSet;
		DescriptorSet lightingDescriptorSet;

		// DescriptorSet dynamicTransformDescriptorSet;
	};

	VkPipelineLayout frameDataDescriptorLayout;
	VkPipelineLayout lightingDescriptorLayout;


	VulkanDevice& device;
	DescriptorManager& descriptor_man;

	std::array<Dynamic, 2> d_buffers;

	int cur_index = 0;
};
