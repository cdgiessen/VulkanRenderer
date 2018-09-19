#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include "Device.h"
#include "vulkan/vulkan.h"

// struct AttachmentDescription {
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
// struct AttachmentReference {
//
//    AttachmentReference(int index, VkImageLayout layout)    {
//        reference.attachment = index;
//	    reference.layout = layout;
//    }
//
//    VkAttachmentReference reference = {};
//};

// struct SubpassDescription {
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
// struct SubpassDependency {
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
// class RenderPass {
// public:
// RenderPass(VulkanDevice& device, VkFormat colorFormat);
//~RenderPass();

// void BeginRenderPass(VkCommandBuffer cmdBuf,
//    VkFramebuffer framebuffer,
//    VkOffset2D offset, VkExtent2D extent,
//    std::array<VkClearValue, 2> clearValues,
//    VkSubpassContents contents);
//
// void NextSubPass (VkCommandBuffer cmdBuf, VkSubpassContents contents);
// void EndRenderPass(VkCommand
// VkRenderPass Get();

// private:
//    VkRenderPass renderPass;
//};



using RenderFunc = std::function<void(VkCommandBuffer cmdBuf)>;

struct RenderPassAttachment
{
	RenderPassAttachment (std::string name, VkFormat format) : name (name), format (format) {}

	std::string name;
	VkFormat format;
	VkAttachmentDescription description = {};
	//VkAttachmentReference reference = {};
};
using AttachmentMap = std::unordered_map<std::string, RenderPassAttachment>;

struct SubpassDependency
{
	SubpassDependency (std::string src, std::string dst) : src_subpass (src), dst_subpass (dst) {}
	std::string src_subpass;
	std::string dst_subpass;

	uint32_t dependentSubpass;
	uint32_t sourceSubpass;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	VkAccessFlags srcAccessMask;
	VkAccessFlags dstAccessMask;
	VkDependencyFlags dependencyFlags;

	VkSubpassDependency Get() {
		VkSubpassDependency desc;
		desc.srcSubpass = sourceSubpass;
		desc.dstSubpass = dependentSubpass;
		desc.srcStageMask = srcStageMask;
		desc.dstStageMask = dstStageMask;
		desc.srcAccessMask = srcAccessMask;
		desc.dstAccessMask = dstAccessMask;
		return desc;
	}
};

struct SubpassDescription
{
	SubpassDescription (std::string name) : name (name) {}

	void AddSubpassDependency (std::string subpass);

	void AddImageInput (std::string name);

	void AddColorOutput (std::string name);

	enum class DepthStencilAccess
	{
		read_write,
		read_only,
		depth_read_only,
		stencil_read_only
	};
	void SetDepthStencil (std::string name, DepthStencilAccess access);

	void AddResolveAttachments (std::string name);
	void AddPreserveAttachments (std::string name);

	std::vector<std::string> AttachmentsUsed (AttachmentMap& const attachment_map) const;

	std::string name;
	uint32_t index;
	std::vector<std::string> subpass_dependencies;

	std::vector<std::string> input_attachments;
	std::vector<std::string> color_attachments;
	std::vector<std::string> resolve_attachments;
	std::optional<std::string> depth_stencil_attachment;
	DepthStencilAccess depth_stencil_access;
	std::vector<std::string> preserve_attachments;
};

struct VulkanSubpassDescription {
	std::vector<VkAttachmentReference> ar_inputs;
	std::vector<VkAttachmentReference> ar_colors;
	std::vector<VkAttachmentReference> ar_resolves;
	std::vector<uint32_t> ar_preserves;
	VkAttachmentReference ar_depth_stencil;

	VkSubpassDescription desc;

	VkSubpassDescription Get() {
		desc.inputAttachmentCount = ar_inputs.size();
		desc.pInputAttachments = ar_inputs.data();

		desc.colorAttachmentCount = ar_colors.size();
		desc.pColorAttachments = ar_colors.data();
		desc.pResolveAttachments = ar_resolves.data();
		
		desc.preserveAttachmentCount = ar_preserves.size();
		desc.pPreserveAttachments = ar_preserves.data();
		
		desc.pDepthStencilAttachment = &ar_depth_stencil;
		return desc;
	}
};


struct RenderPassDescription
{
	RenderPassDescription (std::string name) : name (name) {}

	void AddSubpass (SubpassDescription subpass);

	VkRenderPassCreateInfo GetRenderPassCreate (AttachmentMap& attachment_map);

	std::string name;
	// std::vector< std::string> attachments;
	std::vector<SubpassDescription> subpasses;
};
using RenderPassMap = std::unordered_map<std::string, RenderPassDescription>;

struct RenderPass
{
	RenderPass (VkRenderPass renderPass) : rp (renderPass){};

	void SetSubpassDrawFuncs (std::vector<RenderFunc> funcs);

	void BuildCmdBuf (VkCommandBuffer cmdBuf,
	    VkFramebuffer framebuffer,
	    VkOffset2D offset,
	    VkExtent2D extent,
	    std::array<VkClearValue, 2> clearValues);

	std::vector<RenderFunc> subpassFuncs;
	VkRenderPass rp;
};

struct FrameGraphBuilder
{

	void AddAttachment ( RenderPassAttachment attachment);
	std::unordered_map<std::string, RenderPassAttachment> attachments;

	void AddRenderPass (RenderPassDescription renderPass);
	std::unordered_map<std::string, RenderPassDescription> renderPasses;

	std::string lastPass;
};

class FrameGraph
{
	public:
	FrameGraph (FrameGraphBuilder builder, VulkanDevice& device);
	~FrameGraph ();

	VkRenderPass Get (int index) const;
	void SetDrawFuncs (std::vector<RenderFunc> funcs);

	private:
	VulkanDevice& device;

	std::vector<RenderPass> renderPasses;
};