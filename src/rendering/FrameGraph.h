#pragma once 

#include <string>
#include <unordered_map>
#include <functional>

#include "Device.h"
#include "vulkan/vulkan.h"

//struct AttachmentDescription {
//
//    AttachmentDescription(
//        VkFormat                        format,
//        VkSampleCountFlagBits           samples,
//        VkAttachmentLoadOp              loadOp,
//        VkAttachmentStoreOp             storeOp,
//        VkAttachmentLoadOp              stencilLoadOp,
//        VkAttachmentStoreOp             stencilStoreOp,
//        VkImageLayout                   initialLayout,
//        VkImageLayout                   finalLayout
//    ) {
//        
//	    description.format = format;
//	    description.samples = samples;
//	    description.loadOp = loadOp;
//	    description.storeOp = storeOp;
//	    description.stencilLoadOp = stencilLoadOp;
//	    description.stencilStoreOp = stencilStoreOp;
//	    description.initialLayout = initialLayout;
//	    description.finalLayout = finalLayout;
//    }
//
//
//    VkAttachmentDescription description = {};
//};
//
//struct AttachmentReference {
//
//    AttachmentReference(int index, VkImageLayout layout)    {
//        reference.attachment = index;
//	    reference.layout = layout;
//    }
//
//    VkAttachmentReference reference = {};
//};

//struct SubpassDescription {
//    SubpassDescription (
//        VkSubpassDescriptionFlags       flags,
//        VkPipelineBindPoint             pipelineBindPoint,
//        uint32_t                        inputAttachmentCount,
//        const VkAttachmentReference*    pInputAttachments,
//        uint32_t                        colorAttachmentCount,
//        const VkAttachmentReference*    pColorAttachments,
//        const VkAttachmentReference*    pDepthStencilAttachment,
//        const VkAttachmentReference*    pResolveAttachments,
//        uint32_t                        preserveAttachmentCount,
//        const uint32_t*                 pPreserveAttachments
//    ){
//        subpass.flags = flags;
//        subpass.pipelineBindPoint = pipelineBindPoint;
//        subpass.inputAttachmentCount = inputAttachmentCount;
//        subpass.pInputAttachments = pInputAttachments;
//        subpass.colorAttachmentCount = colorAttachmentCount;
//        subpass.pColorAttachments = pColorAttachments;
//        subpass.pResolveAttachments = pResolveAttachments;
//        subpass.pDepthStencilAttachment = pDepthStencilAttachment;
//        subpass.preserveAttachmentCount = preserveAttachmentCount;
//        subpass.pPreserveAttachments = pPreserveAttachments;
//    }
//
//    VkSubpassDescription subpass = {};
//};
//
//struct SubpassDependency {
//    SubpassDependency(
//        uint32_t                srcSubpass,
//        uint32_t                dstSubpass,
//        VkPipelineStageFlags    srcStageMask,
//        VkPipelineStageFlags    dstStageMask,
//        VkAccessFlags           srcAccessMask,
//        VkAccessFlags           dstAccessMask,
//        VkDependencyFlags       dependencyFlags
//    ) {
//        dependency.srcSubpass       = srcSubpass;
//        dependency.dstSubpass       = dstSubpass;
//        dependency.srcStageMask     = srcStageMask;
//        dependency.dstStageMask     = dstStageMask;
//        dependency.srcAccessMask    = srcAccessMask;
//        dependency.dstAccessMask    = dstAccessMask;
//        dependency.dependencyFlags  = dependencyFlags;
//    }
//    VkSubpassDependency dependency = {};
//};
//class RenderPass {
//public:
	//RenderPass(VulkanDevice& device, VkFormat colorFormat);
	//~RenderPass();

	//void BeginRenderPass(VkCommandBuffer cmdBuf, 
	//    VkFramebuffer framebuffer, 
	//    VkOffset2D offset, VkExtent2D extent, 
	//    std::array<VkClearValue, 2> clearValues, 
	//    VkSubpassContents contents);
	//
	//void NextSubPass (VkCommandBuffer cmdBuf, VkSubpassContents contents);
	//void EndRenderPass(VkCommand 
	//VkRenderPass Get();

//private:
//    VkRenderPass renderPass;
//};



using RenderFunc = std::function<void(VkCommandBuffer cmdBuf)>;
using AttachmentMap = std::unordered_map<std::string, RenderPassAttachment>;
using RenderPassMap = std::unordered_map<std::string, RenderPassDescription>;

struct RenderPassAttachment {
	RenderPassAttachment(std::string name, VkFormat format):
		name(name), format(format) {}

	std::string name;
	VkFormat format;
	VkAttachmentDescription description = {};
	VkAttachmentReference reference = {};
};

struct SubpassDependency {
	SubpassDependency(std::string src, std::string dst):
		src_subpass(src), dst_subpass(dst)

	{}
	std::string src_subpass;
	std::string dst_subpass;

	uint32_t                dependentSubpass;
	uint32_t                sourceSubpass;
	VkPipelineStageFlags    srcStageMask;
	VkPipelineStageFlags    dstStageMask;
	VkAccessFlags           srcAccessMask;
	VkAccessFlags           dstAccessMask;
	VkDependencyFlags       dependencyFlags;
};

struct SubpassDescription {
	SubpassDescription(std::string name):name(name){}

	void AddSubpassDependency(std::string subpass);

	void AddImageInput(std::string name);

	void AddColorOutput(std::string name);

	enum class DepthStencilAccess {read_write, read_only, depth_read_only, stencil_read_only};
	void SetDepthStencil(std::string name, DepthStencilAccess access);

	void AddResolveAttachments(std::string name);
	void AddPreserveAttachments(std::string name);

	std::vector<VkAttachmentDescription> AttachmentsUsed(AttachmentMap& attachment_map) const;
	VkSubpassDescription GetSubpassDescription(AttachmentMap& attachment_map) const;

	std::string name;
	uint32_t index;
	std::vector<std::string> subpass_dependencies;

	std::vector<std::string> input_attachments;
	std::vector<std::string> color_attachments;
	std::vector<std::string> resolve_attachments;
	std::optional<std::string> depth_stencil_attachment; DepthStencilAccess depth_stencil_access;
	std::vector<std::string> preserve_attachments;
};
	



struct RenderPassDescription {
	RenderPassDescription(std::string name):name(name) {}

	void AddSubpass(SubpassDescription subpass);

	VkRenderPassCreateInfo GetRenderPassCreate(AttachmentMap& attachment_map);

	std::string name;
	//std::vector< std::string> attachments;
	std::vector<SubpassDescription> subpasses;
};

struct RenderPass {
	RenderPass(VkRenderPass renderPass):rp(renderPass) {};

	void SetSubpassDrawFuncs(std::vector<RenderFunc> funcs);

	void BuildCmdBuf(VkCommandBuffer cmdBuf, VkFramebuffer framebuffer,
		VkOffset2D offset, VkExtent2D extent, std::array<VkClearValue, 2> clearValues);

	std::vector<RenderFunc> subpassFuncs;
	VkRenderPass rp;
};

struct FrameGraphBuilder {

	void AddAttachment(std::string name, RenderPassAttachment attachment);
	std::unordered_map<std::string, RenderPassAttachment> attachments;

	void AddRenderPass(std::string name, RenderPassDescription renderPass);
	std::unordered_map<std::string, RenderPassDescription> renderPasses;

	std::string lastPass;
};

class FrameGraph {
public:

	FrameGraph(FrameGraphBuilder builder, VulkanDevice& device);
	~FrameGraph();

	VkRenderPass Get(int index) const;

private:
	VulkanDevice& device;

	std::vector<RenderPass> renderPasses;
};