#include "FrameGraph.h"

#include "Initializers.h"

//RenderPass::RenderPass (VulkanDevice& device, VkFormat colorFormat) : device (device)
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

//RenderPass::~RenderPass () { vkDestroyRenderPass (device.device, renderPass, nullptr); }
//
//void RenderPass::BeginRenderPass (VkCommandBuffer cmdBuf,
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
//void RenderPass ::NextSubPass (VkCommandBuffer cmdBuf, VkSubpassContents contents)
//{
//	vkCmdNextSubpass (cmdBuf, contents);
//}
//
//void RenderPass::EndRenderPass (VkCommandBuffer cmdBuf) { vkCmdEndRenderPass (cmdBuf); }


//VkRenderPass RenderPass::Get () { return renderPass; }


VkFormat findColorFormat() { return VK_FORMAT_R8G8B8A8_SNORM; }

VkFormat findDepthFormat() { return VK_FORMAT_D32_SFLOAT_S8_UINT;}

FrameGraph ContrustRenderPass(VulkanDevice& device) {
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work("main_work");

	frame_graph_builder.AddAttachment({ "img_color", findColorFormat() });
	frame_graph_builder.AddAttachment({ "img_depth", findDepthFormat() });

	SubpassDescription depth_subpass("sub_depth");
	depth_subpass.SetDepthStencil("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	main_work.AddSubpass(depth_subpass);

	SubpassDescription color_subpass("sub_color");
	color_subpass.SetDepthStencil("img_depth", SubpassDescription::DepthStencilAccess::read_only);
	color_subpass.AddColorOutput("img_color");
	main_work.AddSubpass(color_subpass);

	frame_graph_builder.AddRenderPass(main_work);


	return FrameGraph(frame_graph_builder, device);
}

void SubpassDescription::AddSubpassDependency(std::string subpass){ subpass_dependencies.push_back(name); }
void SubpassDescription::AddImageInput(std::string name){ input_attachments.push_back(name); }
void SubpassDescription::AddColorOutput(std::string name){ color_attachments.push_back(name); }
void SubpassDescription::AddResolveAttachments(std::string name) { resolve_attachments.push_back(name); }
void SubpassDescription::AddPreserveAttachments(std::string name) { preserve_attachments.push_back(name); }
void SubpassDescription::SetDepthStencil(std::string name, DepthStencilAccess access){ 
	depth_stencil_attachment = name;
	depth_stencil_access = access;
}


void RenderPassDescription::AddSubpass(SubpassDescription subpass) {
	subpasses.push_back(subpass);
}

void FrameGraphBuilder::AddAttachment(RenderPassAttachment attachment) {
	attachments.push_back(attachment);
}

void FrameGraphBuilder::AddRenderPass(RenderPassDescription renderPass) {
	renderPasses.push_back(renderPass);
}

FrameGraph::FrameGraph(FrameGraphBuilder builder, VulkanDevice& device):device(device) {





}

FrameGraph::~FrameGraph() {
	for(auto& rp : renderPasses)
		vkDestroyRenderPass(device.device, rp, nullptr);
}

