#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanTexture;

using RenderFunc = std::function<void (VkCommandBuffer cmdBuf)>;

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

	VkSubpassDependency get ()
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

	void add_subpass_dependency (std::string subpass);

	void add_image_input (std::string name);

	void add_color_output (std::string name);

	void add_clear_color (std::string attachment_name, VkClearValue value);

	enum class DepthStencilAccess
	{
		read_write,
		read_only,
		depth_read_only,
		stencil_read_only
	};
	void set_depth_stencil (std::string name, DepthStencilAccess access);

	void add_resolve_attachments (std::string name);
	void add_preserve_attachments (std::string name);

	void set_function (RenderFunc&& func);

	std::vector<std::string> attachments_used (AttachmentMap const& attachment_map) const;

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

	VkSubpassDescription get ()
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

	VkAttachmentDescription get ();


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

	void add_subpass (SubpassDescription subpass);

	VkRenderPassCreateInfo get_renderpass_create_info (AttachmentMap& attachment_map);
	std::vector<RenderFunc> get_subpass_functions ();
	std::vector<std::string> get_used_attachment_names ();

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
	void add_attachment (RenderPassAttachment attachment);
	void add_render_pass (RenderPassDescription renderPass);
	void set_final_render_pass_name (std::string name);
	void set_final_output_attachment_name (std::string name);

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
	FrameBuffer (FrameBuffer&& fb) noexcept;
	FrameBuffer& operator= (FrameBuffer&& fb) noexcept;


	VkFramebuffer get () const { return framebuffer; }

	VkRect2D get_full_size () const { return { { 0, 0 }, { width, height } }; }

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
	RenderPass (RenderPass&& rp) noexcept;
	RenderPass& operator= (RenderPass&& rp) noexcept;

	void BuildCmdBuf (VkCommandBuffer cmdBuf, FrameBufferView fb_view);
	std::vector<std::string> get_used_attachment_names ()
	{
		return desc.get_used_attachment_names ();
	}

	VkRenderPass get () const { return rp; }

	std::vector<VkImageView> order_attachments (std::vector<std::pair<std::string, VkImageView>> const& named_views);

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

	VkRenderPass get (int index) const;
	VkRenderPass get_present_render_pass () const { return final_renderpass->get (); };

	void create_present_resources ();
	void destroy_present_resources ();

	void fill_command_buffer (VkCommandBuffer cmdBuf, FrameBufferView frame_buffer_view);
	void fill_command_buffer (VkCommandBuffer cmdBuf, std::string frame_buffer);

	void set_current_frame_index (uint32_t index) { current_frame = index; }

	int get_frame_buffer_id (std::string name) const;

	private:
	void create_attachments ();
	void create_framebuffers ();

	FrameBuffer const& get_framebuffer (int index);

	VulkanDevice& device;
	VulkanSwapChain& swapchain;

	std::vector<RenderPass> render_passes;
	std::unique_ptr<RenderPass> final_renderpass;

	FrameGraphBuilder builder; // for later use

	std::unordered_map<std::string, std::unique_ptr<VulkanTexture>> render_targets;

	int current_frame = 0;
	std::vector<FrameBuffer> framebuffers;
	std::vector<FrameBuffer> swapchain_framebuffers;
};
