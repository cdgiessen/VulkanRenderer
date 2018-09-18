#include "FrameGraph.h"

#include "Initializers.h"

// RenderPass::RenderPass (VulkanDevice& device, VkFormat colorFormat) : device (device)
//{
//	AttachmentDescription colorAttachment (colorFormat,
//	    VK_SAMPLE_COUNT_1_BIT,
//	    VK_ATTACHMENT_LOAD_OP_CLEAR,
//	    VK_ATTACHMENT_STORE_OP_STORE,
//	    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//	    VK_ATTACHMENT_STORE_OP_DONT_CARE,
//	    VK_IMAGE_LAYOUT_UNDEFINED,
//	    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
//
//	AttachmentDescription depthAttachment (VK_FORMAT_D32_SFLOAT_S8_UINT,
//	    VK_SAMPLE_COUNT_1_BIT,
//	    VK_ATTACHMENT_LOAD_OP_CLEAR,
//	    VK_ATTACHMENT_STORE_OP_DONT_CARE,
//	    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//	    VK_ATTACHMENT_STORE_OP_DONT_CARE,
//	    VK_IMAGE_LAYOUT_UNDEFINED,
//	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
//
//	AttachmentReference colorAttachmentRef (0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//	AttachmentReference depthAttachmentRef (1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
//
//	SubpassDescription depth_subpass (0,
//	    VK_PIPELINE_BIND_POINT_GRAPHICS,
//	    0,
//	    nullptr,
//	    1,
//		nullptr,
//	    &depthAttachmentRef.reference,
//	    nullptr,
//	    0,
//	    nullptr);
//
//	SubpassDescription color_subpass (0,
//	    VK_PIPELINE_BIND_POINT_GRAPHICS,
//	    1,
//	    &depthAttachmentRef.reference,
//	    1,
//	    &colorAttachmentRef.reference,
//		nullptr,
//	    nullptr,
//	    0,
//	    nullptr);
//
//	SubpassDependency ext_subpass_dependency (VK_SUBPASS_EXTERNAL,
//	    0,
//	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
//	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//	    0,
//	    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//	    0);
//
//	SubpassDependency depth_subpass_dependency (0,
//	    1,
//	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//	    0,
//	    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//	    0);
//
//	SubpassDependency color_subpass_dependency (1,
//	    VK_SUBPASS_EXTERNAL,
//	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//	    0,
//	    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//	    0);
//
//
//	std::array<VkSubpassDependency, 3> subpass_dependencies = { ext_subpass_dependency.dependency,
//		depth_subpass_dependency.dependency,
//		color_subpass_dependency.dependency };
//
//	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment.description,
//		depthAttachment.description };
//
//	std::array<VkSubpassDescription, 2> subpass_descriptions = { depth_subpass.subpass,
//		color_subpass.subpass };
//
//	VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo ();
//	renderPassInfo.attachmentCount = static_cast<uint32_t> (attachments.size ());
//	renderPassInfo.pAttachments = attachments.data ();
//	renderPassInfo.subpassCount = 2;
//	renderPassInfo.pSubpasses = subpass_descriptions.data ();
//	renderPassInfo.dependencyCount = 3;
//	renderPassInfo.pDependencies = subpass_dependencies.data ();
//
//	if (vkCreateRenderPass (device.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
//	{
//		throw std::runtime_error ("failed to create render pass!");
//	}

// VkAttachmentDescription colorAttachment = {};
// colorAttachment.format = colorFormat;
// colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
// colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
// colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
// colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
// colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
// colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

// VkAttachmentDescription depthAttachment = {};
// depthAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // findDepthFormat();
// depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
// depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
// depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
// depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
// depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

// VkAttachmentReference colorAttachmentRef = {};
// colorAttachmentRef.attachment = 0;
// colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

// VkAttachmentReference depthAttachmentRef = {};
// depthAttachmentRef.attachment = 1;
// depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

// VkSubpassDescription subpass = {};
// subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
// subpass.colorAttachmentCount = 1;
// subpass.pColorAttachments = &colorAttachmentRef;
// subpass.pDepthStencilAttachment = &depthAttachmentRef;

// VkSubpassDependency dependency = {};
// dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
// dependency.dstSubpass = 0;
// dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
// dependency.srcAccessMask = 0;
// dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
// dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
// 	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

// std::array<VkAttachmentDescription, 2> attachments = { colorAttachment,
// 													  depthAttachment };
// VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo();
// renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
// renderPassInfo.pAttachments = attachments.data();
// renderPassInfo.subpassCount = 1;
// renderPassInfo.pSubpasses = &subpass;
// renderPassInfo.dependencyCount = 1;
// renderPassInfo.pDependencies = &dependency;

// if (vkCreateRenderPass(device.device, &renderPassInfo, nullptr,
// 	&renderPass) != VK_SUCCESS) {
// 	throw std::runtime_error("failed to create render pass!");
// }
//}

// RenderPass::~RenderPass () { vkDestroyRenderPass (device.device, renderPass, nullptr); }
//
// void RenderPass::BeginRenderPass (VkCommandBuffer cmdBuf,
//    VkFramebuffer framebuffer,
//    VkOffset2D offset,
//    VkExtent2D extent,
//    std::array<VkClearValue, 2> clearValues,
//    VkSubpassContents contents)
//{
//	VkRenderPassBeginInfo renderPassInfo =
//	    initializers::renderPassBeginInfo (renderPass, framebuffer, offset, extent, clearValues);
//
//	vkCmdBeginRenderPass (cmdBuf, &renderPassInfo, contents);
//}
//
//
// void RenderPass ::NextSubPass (VkCommandBuffer cmdBuf, VkSubpassContents contents)
//{
//	vkCmdNextSubpass (cmdBuf, contents);
//}
//
// void RenderPass::EndRenderPass (VkCommandBuffer cmdBuf) { vkCmdEndRenderPass (cmdBuf); }


// VkRenderPass RenderPass::Get () { return renderPass; }


VkFormat findColorFormat () { return VK_FORMAT_R8G8B8A8_SNORM; }

VkFormat findDepthFormat () { return VK_FORMAT_D32_SFLOAT_S8_UINT; }

FrameGraph ContrustRenderPass (VulkanDevice& device)
{
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work ("main_work");

	frame_graph_builder.AddAttachment ({ "img_color", findColorFormat () });
	frame_graph_builder.AddAttachment ({ "img_depth", findDepthFormat () });

	SubpassDescription depth_subpass ("sub_depth");
	depth_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	main_work.AddSubpass (depth_subpass);

	SubpassDescription color_subpass ("sub_color");
	color_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_only);
	color_subpass.AddColorOutput ("img_color");
	color_subpass.AddSubpassDependency ("sub_depth");
	main_work.AddSubpass (color_subpass);

	frame_graph_builder.AddRenderPass (main_work);
	frame_graph_builder.lastPass = main_work.name;

	return FrameGraph (frame_graph_builder, device);
}

void SubpassDescription::AddSubpassDependency (std::string subpass)
{
	subpass_dependencies.push_back (name);
}
void SubpassDescription::AddImageInput (std::string name) { input_attachments.push_back (name); }
void SubpassDescription::AddColorOutput (std::string name) { color_attachments.push_back (name); }
void SubpassDescription::AddResolveAttachments (std::string name)
{
	resolve_attachments.push_back (name);
}
void SubpassDescription::AddPreserveAttachments (std::string name)
{
	preserve_attachments.push_back (name);
}
void SubpassDescription::SetDepthStencil (std::string name, DepthStencilAccess access)
{
	depth_stencil_attachment = name;
	depth_stencil_access = access;
}

std::vector<std::string> SubpassDescription::AttachmentsUsed (AttachmentMap& attachment_map) const
{
	std::vector<std::string> attachments;
	for (auto& item : input_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : color_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : resolve_attachments)
	{
		attachments.push_back (item);
	}
	for (auto& item : preserve_attachments)
	{
		attachments.push_back (item);
	}
	if (depth_stencil_attachment.has_value ()) attachments.push_back (*depth_stencil_attachment);
	return attachments;
}

void RenderPassDescription::AddSubpass (SubpassDescription subpass)
{
	subpasses.push_back (subpass);
}

VkRenderPassCreateInfo RenderPassDescription::GetRenderPassCreate (AttachmentMap& attachment_map)
{
	// Get all used attachments from subpasses(ignoring duplicate usages with std::unordered_set)
	std::unordered_set<std::string> used_attachment_names;
	for (auto& rp_subpass : pass.subpasses)
	{
		auto& sub_attaches = rp_subpass.AttachmentsUsed ();
		for (auto& attach_name : sub_attaches)
		{
			used_attachment_names.insert (name);
		}
	}
	std::vector<VkAttachmentDescription> rp_attachments;
	for (auto& attach_name : used_attachment_names)
	{
		if (attachment_map.count (attach_name)) == 1 ){
			rp_attachments.push_back(attachment_map.at(attach_name));
		}
	}

	// create subpasses

	std::vector<VkSubpassDescription> sb_descriptions;
	for (auto& rp_subpass : pass.subpasses)
	{
		//	auto subpass = rp_subpass.GetSubpassDescription ();

		std::vector<VkAttachmentReference> ar_inputs;
		std::vector<VkAttachmentReference> ar_colors;
		std::vector<VkAttachmentReference> ar_resolves;
		std::vector<VkAttachmentReference> ar_preserves;
		VkAttachmentReference ar_depth_stencil;
		for (auto& name : p_subpass.input_attachments)
		{
			auto sb_a = attachment_map.at (name);
			VkAttachmentReference ref;
			ref.layout = sb_a.layout;

			ar_inputs.push_back (attachment_map.at (name))
		}

		VkSubpassDescription desc = {};
		desc.inputAttachmentCount = rp_subpass.input_attachments.size ();
		desc.colorAttachmentCount = rp_subpass.color_attachments.size ();
		desc.preserveAttachmentCount = rp_subpass.preserve_attachments.size ();


		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		sb_descriptions.push_back (desc);
	}

	// create subpass dependencies
	std::vector<SubpassDependency> sb_dependencies;


	VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo ();
	renderPassInfo.attachmentCount = static_cast<uint32_t> (rp_attachments.size ());
	renderPassInfo.pAttachments = rp_attachments.data ();
	renderPassInfo.subpassCount = sb_descriptions.size ();
	renderPassInfo.pSubpasses = sb_descriptions.data ();
	renderPassInfo.dependencyCount = sb_dependencies.size ();
	renderPassInfo.pDependencies = sb_dependencies.data ();

	return renderPassInfo;
}



void FrameGraphBuilder::AddAttachment (std::string name, RenderPassAttachment attachment)
{

	attachments[name] = attachment;
}

void FrameGraphBuilder::AddRenderPass (std::string name, RenderPassDescription renderPass)
{
	renderPasses[name] = renderPass;
}

FrameGraph::FrameGraph (FrameGraphBuilder builder, VulkanDevice& device) : device (device)
{

	for (auto [name, pass] : builder.renderPasses)
	{

		auto renderPassInfo = pass.GetRenderPassCreate ();


		VkRenderPass pass;
		if (vkCreateRenderPass (device.device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS)
		{
			throw std::runtime_error ("failed to create render pass!");
		}
		renderPasses.push_back (RenderPass (pass));
	}
}

FrameGraph::~FrameGraph ()
{
	for (auto& rp : renderPasses)
		vkDestroyRenderPass (device.device, rp.rp, nullptr);
}

void RenderPass::SetSubpassDrawFuncs (std::vector<std::function<void(VkCommandBuffer cmdBuf)>> funcs)
{
	subpassFuncs = funcs;
}

void RenderPass::BuildCmdBuf (VkCommandBuffer cmdBuf,
    VkFramebuffer framebuffer,
    VkOffset2D offset,
    VkExtent2D extent,
    std::array<VkClearValue, 2> clearValues)
{
	VkRenderPassBeginInfo renderPassInfo =
	    initializers::renderPassBeginInfo (rp, framebuffer, offset, extent, clearValues);


	vkCmdBeginRenderPass (cmdBuf, &renderPassInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	for (auto& func : subpassFuncs)
	{
		func (cmdBuf);
		vkCmdNextSubpass (cmdBuf, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
	}
	vkCmdEndRenderPass (cmdBuf);
}
