#pragma once 

#include "Device.h"

#include "vulkan/vulkan.h"



class RenderPass {
public:
    RenderPass(VulkanDevice& device, VkFormat colorFormat);
    ~RenderPass();

    void BeginRenderPass(VkCommandBuffer cmdBuf, 
        VkFramebuffer framebuffer, 
        VkOffset2D offset, VkExtent2D extent, 
        std::array<VkClearValue, 2> clearValues, 
        VkSubpassContents contents);

    void EndRenderPass(VkCommandBuffer cmdBuf);

    VkRenderPass Get();

private:
    VulkanDevice& device;

    VkRenderPass renderPass;

};