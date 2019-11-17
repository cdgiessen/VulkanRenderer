#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "SG14/flat_map.h"

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanTexture;

using RenderFunc = std::function<void(VkCommandBuffer cmdBuf)>;

struct RenderPassAttachment
{
	std::string name;
	VkFormat format;
	VkAttachmentDescription description = {};
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t layers = 1;
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

	VkSubpassDependency Get ()
	{
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

	void AddClearColor (std::string attachment_name, VkClearValue value);

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

	void SetFunction (RenderFunc&& func);

	std::vector<std::string> AttachmentsUsed (AttachmentMap const& attachment_map) const;

	std::string name;
	uint32_t index;
	std::vector<std::string> subpass_dependencies;

	std::vector<std::string> input_attachments;
	std::vector<std::string> color_attachments;
	std::vector<std::string> resolve_attachments;
	std::optional<std::string> depth_stencil_attachment;
	DepthStencilAccess depth_stencil_access;
	std::vector<std::string> preserve_attachments;

	std::unordered_map<std::string, VkClearValue> clear_values;

	RenderFunc func;
};

struct VulkanSubpassDescription
{
	std::vector<VkAttachmentReference> ar_inputs;
	std::vector<VkAttachmentReference> ar_colors;
	std::vector<VkAttachmentReference> ar_resolves;
	std::vector<uint32_t> ar_preserves;
	bool has_depth_stencil = false;
	VkAttachmentReference ar_depth_stencil;

	VkSubpassDescription desc;

	VkSubpassDescription Get ()
	{
		desc.flags = 0;

		desc.inputAttachmentCount = (uint32_t)ar_inputs.size ();
		desc.pInputAttachments = ar_inputs.data ();

		desc.colorAttachmentCount = (uint32_t)ar_colors.size ();
		desc.pColorAttachments = ar_colors.data ();
		desc.pResolveAttachments = ar_resolves.data ();

		desc.preserveAttachmentCount = (uint32_t)ar_preserves.size ();
		desc.pPreserveAttachments = ar_preserves.data ();

		if (has_depth_stencil)
		{
			desc.pDepthStencilAttachment = &ar_depth_stencil;
		}

		return desc;
	}
};

struct AttachmentUse
{
	AttachmentUse (RenderPassAttachment rpAttach, uint32_t index);

	VkAttachmentDescription Get ();


	VkFormat format;
	RenderPassAttachment rpAttach;
	uint32_t index = -1;
	VkSampleCountFlagBits sampleCount;
	VkAttachmentLoadOp loadOp;
	VkAttachmentStoreOp storeOp;
	VkAttachmentLoadOp stencilLoadOp;
	VkAttachmentStoreOp stencilStoreOp;
	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

struct RenderPassDescription
{
	RenderPassDescription (std::string name = "default") : name (name) {}

	void AddSubpass (SubpassDescription subpass);

	VkRenderPassCreateInfo GetRenderPassCreate (AttachmentMap& attachment_map);
	std::vector<RenderFunc> GetSubpassFunctions ();
	std::vector<std::string> GetUsedAttachmentNames ();

	bool present_attachment = false;
	std::string name;
	std::vector<SubpassDescription> subpasses;

	// needed for the call to create_render_pass
	std::vector<VulkanSubpassDescription> vulkan_sb_descriptions;
	std::vector<VkAttachmentDescription> rp_attachments;
	std::vector<VkSubpassDescription> sb_descriptions;
	std::vector<VkSubpassDependency> sb_dependencies;

	std::vector<AttachmentUse> attachment_uses;
	std::vector<VkClearValue> clear_values;

	std::vector<RenderFunc> functions;
};
using RenderPassMap = std::unordered_map<std::string, RenderPassDescription>;
class FrameGraph;

struct FrameGraphBuilder
{
	void AddAttachment (RenderPassAttachment attachment);
	void AddRenderPass (RenderPassDescription renderPass);
	void SetFinalRenderPassName (std::string name);
	void SetFinalOutputAttachmentName (std::string name);

	friend FrameGraph;

	private:
	std::unordered_map<std::string, RenderPassAttachment> attachments;
	std::unordered_map<std::string, RenderPassDescription> render_passes;
	std::string final_renderpass;
	std::string final_output_attachment;
};

class FrameBuffer
{
	public:
	FrameBuffer (VulkanDevice& device,
	    std::vector<VkImageView> image_views,
	    VkRenderPass renderPass,
	    uint32_t width,
	    uint32_t height,
	    uint32_t layers);
	~FrameBuffer ();
	FrameBuffer (FrameBuffer const& fb) = delete;
	FrameBuffer& operator= (FrameBuffer const& fb) = delete;
	FrameBuffer (FrameBuffer&& fb);
	FrameBuffer& operator= (FrameBuffer&& fb);


	VkFramebuffer Get () const { return framebuffer; }

	VkRect2D GetFullSize () const { return { { 0, 0 }, { width, height } }; }

	private:
	VkDevice device;

	VkFramebuffer framebuffer;
	uint32_t width;
	uint32_t height;
};

struct FrameBufferView
{
	FrameBufferView (VkFramebuffer frame_buffer, VkRect2D view) : fb (frame_buffer), view (view) {}

	VkFramebuffer fb;
	VkRect2D view;
};

class RenderPass
{
	public:
	RenderPass (VkDevice device, RenderPassDescription desc, AttachmentMap& attachments);
	~RenderPass ();
	RenderPass (RenderPass const& rp) = delete;
	RenderPass& operator= (RenderPass const& rp) = delete;
	RenderPass (RenderPass&& rp);
	RenderPass& operator= (RenderPass&& rp);

	void BuildCmdBuf (VkCommandBuffer cmdBuf, FrameBufferView fb_view);
	std::vector<std::string> GetUsedAttachmentNames () { return desc.GetUsedAttachmentNames (); }

	VkRenderPass Get () const { return rp; }

	std::vector<VkImageView> OrderAttachments (std::vector<std::pair<std::string, VkImageView>> const& named_views);

	private:
	VkDevice device;
	std::vector<RenderFunc> subpassFuncs;
	VkRenderPass rp;

	RenderPassDescription desc;
};

class FrameGraph
{
	public:
	FrameGraph (VulkanDevice& device, VulkanSwapChain& swapchain, FrameGraphBuilder builder);
	~FrameGraph ();

	VkRenderPass Get (int index) const;
	VkRenderPass GetPresentRenderPass () const { return final_renderpass->Get (); };

	VkFramebuffer GetFrameBuffer (std::string name) const { return framebuffers.at (name).Get (); }

	void RecreatePresentResources ();

	void FillCommandBuffer (VkCommandBuffer cmdBuf, FrameBufferView frame_buffer_view);
	void FillCommandBuffer (VkCommandBuffer cmdBuf, std::string frame_buffer);

	void SetCurrentFrameIndex (uint32_t index) { current_frame = index; }

	private:
	void CreatePresentResources ();

	FrameBuffer const& GetFrameBuffer (std::string const& name);

	VulkanDevice& device;
	VulkanSwapChain& swapchain;

	std::vector<RenderPass> render_passes;
	std::unique_ptr<RenderPass> final_renderpass;

	FrameGraphBuilder builder; // for later use

	std::unordered_map<std::string, std::unique_ptr<VulkanTexture>> render_targets;

	int current_frame = 0;
	stdext::flat_map<std::string, FrameBuffer> framebuffers;
	std::vector<FrameBuffer> swapchain_framebuffers;
};
