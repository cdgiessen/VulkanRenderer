#pragma once 

#include "Device.h"

#include "vulkan/vulkan.h"

struct AttachmentDescription {

    AttachmentDescription(
        VkFormat                        format,
        VkSampleCountFlagBits           samples,
        VkAttachmentLoadOp              loadOp,
        VkAttachmentStoreOp             storeOp,
        VkAttachmentLoadOp              stencilLoadOp,
        VkAttachmentStoreOp             stencilStoreOp,
        VkImageLayout                   initialLayout,
        VkImageLayout                   finalLayout
    ) {
        
	    description.format = format;
	    description.samples = samples;
	    description.loadOp = loadOp;
	    description.storeOp = storeOp;
	    description.stencilLoadOp = stencilLoadOp;
	    description.stencilStoreOp = stencilStoreOp;
	    description.initialLayout = initialLayout;
	    description.finalLayout = finalLayout;
    }


    VkAttachmentDescription description = {};
};

struct AttachmentReference {

    AttachmentReference(int index, VkImageLayout layout)    {
        reference.attachment = index;
	    reference.layout = layout;
    }

    VkAttachmentReference reference = {};
};

struct SubpassDescription {
    SubpassDescription (
        VkSubpassDescriptionFlags       flags,
        VkPipelineBindPoint             pipelineBindPoint,
        uint32_t                        inputAttachmentCount,
        const VkAttachmentReference*    pInputAttachments,
        uint32_t                        colorAttachmentCount,
        const VkAttachmentReference*    pColorAttachments,
        const VkAttachmentReference*    pDepthStencilAttachment,
        const VkAttachmentReference*    pResolveAttachments,
        uint32_t                        preserveAttachmentCount,
        const uint32_t*                 pPreserveAttachments
    ){
        subpass.flags = flags;
        subpass.pipelineBindPoint = pipelineBindPoint;
        subpass.inputAttachmentCount = inputAttachmentCount;
        subpass.pInputAttachments = pInputAttachments;
        subpass.colorAttachmentCount = colorAttachmentCount;
        subpass.pColorAttachments = pColorAttachments;
        subpass.pResolveAttachments = pResolveAttachments;
        subpass.pDepthStencilAttachment = pDepthStencilAttachment;
        subpass.preserveAttachmentCount = preserveAttachmentCount;
        subpass.pPreserveAttachments = pPreserveAttachments;
    }

    VkSubpassDescription subpass = {};
};

struct SubpassDependency {
    SubpassDependency(
        uint32_t                srcSubpass,
        uint32_t                dstSubpass,
        VkPipelineStageFlags    srcStageMask,
        VkPipelineStageFlags    dstStageMask,
        VkAccessFlags           srcAccessMask,
        VkAccessFlags           dstAccessMask,
        VkDependencyFlags       dependencyFlags
    ) {
        dependency.srcSubpass       = srcSubpass;
        dependency.dstSubpass       = dstSubpass;
        dependency.srcStageMask     = srcStageMask;
        dependency.dstStageMask     = dstStageMask;
        dependency.srcAccessMask    = srcAccessMask;
        dependency.dstAccessMask    = dstAccessMask;
        dependency.dependencyFlags  = dependencyFlags;
    }
    VkSubpassDependency dependency = {};
};
	


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