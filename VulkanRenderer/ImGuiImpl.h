//#pragma once
//
//#include <glm\common.hpp>
//
//#include <vulkan\vulkan.h>
//#include "VulkanInitializers.hpp"
//#include "VulkanTools.h"
//#include "VulkanBuffer.hpp"
//#include "VulkanDevice.hpp"
//
//// ----------------------------------------------------------------------------
//// ImGUI class
//// ----------------------------------------------------------------------------
//class ImGUI {
//private:
//	// Vulkan resources for rendering the UI
//	VkSampler sampler;
//	VulkanBuffer vertexBuffer;
//	VulkanBuffer indexBuffer;
//	int32_t vertexCount = 0;
//	int32_t indexCount = 0;
//	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
//	VkImage fontImage = VK_NULL_HANDLE;
//	VkImageView fontView = VK_NULL_HANDLE;
//	VkPipelineCache pipelineCache;
//	VkPipelineLayout pipelineLayout;
//	VkPipeline pipeline;
//	VkDescriptorPool descriptorPool;
//	VkDescriptorSetLayout descriptorSetLayout;
//	VkDescriptorSet descriptorSet;
//	VkShaderModule vertexShaderModule;
//	VkShaderModule fragmentShaderModule;
//	VulkanDevice *device;
//
//public:
//	// UI params are set via push constants
//	struct ImGuiPushConstBlock {
//		glm::vec2 scale;
//		glm::vec2 translate;
//	} pushConstBlock;
//
//	ImGUI();
//	~ImGUI();
//
//	// Initialize styles, keys, etc.
//	void init(float width, float height);
//
//	// Initialize all Vulkan resources used by the ui
//	void initResources(VkRenderPass renderPass, VkQueue copyQueue);
//
//	// Update vertex and index buffer containing the imGui elements when required
//	void updateBuffers();
//
//	// Draw current imGui frame into a command buffer
//	void drawFrame(VkCommandBuffer commandBuffer);
//
//};