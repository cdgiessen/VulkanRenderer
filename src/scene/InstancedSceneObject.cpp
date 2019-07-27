#include "InstancedSceneObject.h"

#include "ImGui/imgui.h"
#include "core/Logger.h"
#include "rendering/Initializers.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1


InstancedSceneObject::InstancedSceneObject (VulkanRenderer& renderer, int maxInstances)
: renderer (renderer), maxInstanceCount (maxInstances), instanceCount (0), instancesData (maxInstances)
{
	isFinishedTransfer = std::make_shared<bool> (false);
}


InstancedSceneObject::~InstancedSceneObject () {}


void InstancedSceneObject::InitInstancedSceneObject ()
{
	SetupUniformBuffer ();
	SetupImage ();
	SetupModel ();
	SetupDescriptor ();
	SetupPipeline ();
}

void InstancedSceneObject::LoadModel (std::string filename)
{
	// mesh = std::make_shared<MeshData> ();
	// this->mesh->importFromFile(filename);
}

void InstancedSceneObject::LoadModel (std::shared_ptr<MeshData> mesh) { this->mesh = mesh; }

void InstancedSceneObject::LoadTexture (Resource::Texture::TexID tex) { this->texture = tex; }

void InstancedSceneObject::SetFragmentShaderToUse (std::string frag) { fragShaderPath = frag; }

void InstancedSceneObject::SetCullMode (VkCullModeFlagBits cullMode)
{
	cullModeFlagBits = cullMode;
}

void InstancedSceneObject::SetBlendMode (VkBool32 blendEnable) { enableBlending = blendEnable; }

void InstancedSceneObject::SetupUniformBuffer ()
{
	uniformBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (ModelBufferObject));

	/*uniformBuffer->CreateUniformBuffer(sizeof(ModelBufferObject));*/

	ModelBufferObject ubo = {};
	ubo.model = cml::mat4f (1.0f);
	ubo.model = ubo.model.translate (cml::vec3f (0, 0, 0));
	// ubo.model = cml::rotate(ubo.model, time / 2.0f, cml::vec3f(0.5, 1, 0));
	ubo.normal = cml::to_mat4 (cml::to_mat3 (ubo.model).inverse ().transpose ());

	uniformBuffer->CopyToBuffer (&ubo, sizeof (ModelBufferObject));

	// VK_CHECK_RESULT(uniformBuffer.map(renderer.device.device));
	// uniformBuffer.copyTo(&ubo, sizeof(ModelBufferObject));
	// uniformBuffer.unmap();

	instanceBuffer = std::make_shared<VulkanBufferInstancePersistant> (
	    renderer.device, maxInstanceCount, instanceMemberSize);
	// instanceBuffer->CreatePersistantInstanceBuffer(maxInstanceCount, instanceMemberSize);
	// UploadData();
}

void InstancedSceneObject::SetupImage ()
{
	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8);
	vulkanTexture = renderer.textureManager.CreateTexture2D (texture, details);
}

void InstancedSceneObject::SetupModel ()
{
	vulkanModel = std::make_shared<VulkanModel> (renderer, mesh);
}

void InstancedSceneObject::SetupDescriptor ()
{
	descriptor = renderer.GetVulkanDescriptor ();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	descriptor->SetupLayout (m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool (poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet ();

	std::vector<DescriptorUse> writes;
	writes.push_back (DescriptorUse (0, 1, uniformBuffer->resource));
	writes.push_back (DescriptorUse (1, 1, vulkanTexture->resource));
	descriptor->UpdateDescriptorSet (m_descriptorSet, writes);
}

void InstancedSceneObject::SetupPipeline ()
{
	PipelineOutline out;

	auto vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/instancedSceneObject.vert.spv", ShaderType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/instancedSceneObject.frag.spv", ShaderType::fragment);

	ShaderModuleSet shader_set;
	shader_set.Vertex (vert).Fragment (frag);
	out.SetShaderModuleSet (shader_set);

	VertexLayout layout (Vert_PosNormUv);

	out.AddVertexLayouts (layout.bindingDesc, layout.attribDesc);


	std::vector<VkVertexInputBindingDescription> instanceBufferBinding = { initializers::vertexInputBindingDescription (
		INSTANCE_BUFFER_BIND_ID, sizeof (InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE) };

	std::vector<VkVertexInputAttributeDescription> instanceBufferAttribute = {
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, 0), // Location 4: Position
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3), // Location 5: Rotation
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32_SFLOAT, sizeof (float) * 6), // Location 6: Scale
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SINT, sizeof (float) * 7), // Location 7: Texture array layer index
	};

	out.AddVertexLayouts (instanceBufferBinding, instanceBufferAttribute);

	out.SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

	out.AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	out.AddScissor (1, 1, 0, 0);

	out.SetRasterizer (
	    VK_POLYGON_MODE_FILL, cullModeFlagBits, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	out.SetMultisampling (VK_SAMPLE_COUNT_1_BIT);
	out.SetDepthStencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	out.AddColorBlendingAttachment (enableBlending,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);

	out.AddDescriptorLayouts (renderer.GetGlobalLayouts ());
	out.AddDescriptorLayout (descriptor->GetLayout ());

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	normal = std::make_unique<Pipeline> (renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));

	out.SetRasterizer (
	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	wireframe = std::make_unique<Pipeline> (
	    renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));


	// VulkanPipeline &pipeMan = renderer.pipelineManager;
	// mvp = pipeMan.CreateManagedPipeline();

	// //pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/instancedSceneObject.vert.spv"));
	// //pipeMan.SetFragmentShader(mvp, fragShaderModule);

	// auto vert = renderer.shaderManager.loadShaderModule("assets/shaders/instancedSceneObject.vert.spv", ShaderType::vertex);
	// auto frag = renderer.shaderManager.loadShaderModule(fragShaderPath, ShaderType::fragment);

	// ShaderModuleSet set(vert, frag, {}, {}, {});
	// pipeMan.SetShaderModuleSet(mvp, set);

	// pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(),
	// Vertex::getAttributeDescriptions());
	// pipeMan.SetInputAssembly (mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	// pipeMan.SetViewport (mvp,
	//    (float)renderer.vulkanSwapChain.swapChainExtent.width,
	//    (float)renderer.vulkanSwapChain.swapChainExtent.height,
	//    0.0f,
	//    1.0f,
	//    0.0f,
	//    0.0f);
	// pipeMan.SetScissor (mvp,
	//    renderer.vulkanSwapChain.swapChainExtent.width,
	//    renderer.vulkanSwapChain.swapChainExtent.height,
	//    0,
	//    0);
	// pipeMan.SetViewportState (mvp, 1, 1, 0);
	// pipeMan.SetRasterizer (
	//    mvp, VK_POLYGON_MODE_FILL, cullModeFlagBits, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	// pipeMan.SetMultisampling (mvp, VK_SAMPLE_COUNT_1_BIT);
	// pipeMan.SetDepthStencil (mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	// pipeMan.SetColorBlendingAttachment (mvp,
	//    enableBlending,
	//    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	//    VK_BLEND_OP_ADD,
	//    VK_BLEND_FACTOR_SRC_COLOR,
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	//    VK_BLEND_OP_ADD,
	//    VK_BLEND_FACTOR_SRC_ALPHA,
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	// pipeMan.SetColorBlending (mvp, 1, &mvp->pco.colorBlendAttachment);

	// std::vector<VkDynamicState> dynamicStateEnables = {
	// 	VK_DYNAMIC_STATE_VIEWPORT,
	// 	VK_DYNAMIC_STATE_SCISSOR,
	// };

	// pipeMan.SetDynamicState(mvp, dynamicStateEnables);

	// std::vector<VkDescriptorSetLayout> layouts;
	// renderer.AddGlobalLayouts(layouts);
	// layouts.push_back(descriptor->GetLayout());
	// pipeMan.SetDescriptorSetLayout(mvp, layouts);



	// std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
	// 	// Binding point 0: Mesh vertex layout description at per-vertex rate
	// 	initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex_PosNormTexColor), VK_VERTEX_INPUT_RATE_VERTEX),
	// 	// Binding point 1: Instanced data at per-instance rate
	// 	initializers::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
	// };

	// Vertex attribute bindings
	// Note that the shader declaration for per-vertex and per-instance attributes is the same, the
	// different input rates are only stored in the bindings: instanced.vert:
	//	layout (location = 0) in vec3 inPos;			Per-Vertex
	//	...
	//	layout (location = 4) in vec3 instancePos;	Per-Instance
	// std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
	// 	// Per-vertex attributes
	// 	// These are advanced for each vertex fetched by the vertex shader
	// 	initializers::vertexInputAttributeDescription (
	// 	    VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, 0), // Location 0: Position
	// 	initializers::vertexInputAttributeDescription (
	// 	    VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3), // Location 1: Normal
	// 	initializers::vertexInputAttributeDescription (
	// 	    VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, sizeof (float) * 6), // Location 2: Texture coordinates
	// 	initializers::vertexInputAttributeDescription (
	// 	    VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 8), // Location 3: Color

	// 	// Per-Instance attributes
	// 	// These are fetched for each instance rendered
	// 	initializers::vertexInputAttributeDescription (
	// 	    INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, 0), // Location 4: Position
	// 	initializers::vertexInputAttributeDescription (
	// 	    INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32_SFLOAT, sizeof (float) * 3), // Location 5: Rotation
	// 	initializers::vertexInputAttributeDescription (
	// 	    INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SFLOAT, sizeof (float) * 6), // Location 6: Scale
	// 	initializers::vertexInputAttributeDescription (INSTANCE_BUFFER_BIND_ID,
	// 	    7,
	// 	    VK_FORMAT_R32_SINT,
	// 	    sizeof (float) * 7), // Location 7: Texture array layer index
	// };

	// pipeMan.SetVertexInput(mvp, bindingDescriptions, attributeDescriptions);

	// pipeMan.BuildPipelineLayout(mvp);
	// pipeMan.BuildPipeline(mvp, renderer.renderPass->Get(), 0);

	// pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	// pipeMan.BuildPipeline(mvp, renderer.renderPass->Get(), 0);

	// pipeMan.CleanShaderResources(mvp);
	// pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device,
	// "assets/shaders/normalVecDebug.vert.spv")); pipeMan.SetFragmentShader(mvp,
	// loadShaderModule(renderer.device.device, "assets/shaders/normalVecDebug.frag.spv")); pipeMan.SetGeometryShader(mvp,
	// loadShaderModule(renderer.device.device, "assets/shaders/normalVecDebug.geom.spv"));
	//
	// pipeMan.BuildPipeline(mvp, renderer.renderPass->Get(), 0);
	// pipeMan.CleanShaderResources(mvp);
}

void InstancedSceneObject::UploadInstances ()
{
	// std::lock_guard<std::mutex> lk(instanceDataLock);

	if (isDirty)
	{


		size_t instanceBufferSize = instancesData.size () * sizeof (InstanceData);
		/*

		VulkanBufferInstance stagingBuffer(renderer.device);
		stagingBuffer.CreateStagingInstanceBuffer(instancesData.data(), instancesData.size(), instanceMemberSize);

		TransferCommandWork transfer;
		transfer.work = std::function<void(VkCommandBuffer cmdBuf)>(
		    [=](VkCommandBuffer cmdBuf) {
		    VkBufferCopy copyRegion = {};
		    copyRegion.size = instanceBufferSize;
		    vkCmdCopyBuffer(
		        cmdBuf,
		        stagingBuffer.buffer.buffer,
		        instanceBuffer->buffer.buffer,
		        1,
		        &copyRegion);
		});
		transfer.buffersToClean.push_back(stagingBuffer);
		transfer.flags.push_back(isFinishedTransfer);
		renderer.SubmitTransferWork(std::move(transfer));*/

		// auto copyCmd = renderer.GetTransferCommandBuffer();

		// VkBufferCopy copyRegion = {};
		// copyRegion.size = instanceBufferSize;
		// vkCmdCopyBuffer(
		//	copyCmd,
		//	stagingBuffer.buffer.buffer,
		//	instanceBuffer->buffer.buffer,
		//	1,
		//	&copyRegion);
		//
		// renderer.SubmitTransferCommandBufferAndWait(copyCmd);

		// Not sure if needed since the amount of data should be baked in, not current instance count
		// instanceBuffer->resource.FillResource(instanceBuffer->buffer.buffer, 0, instanceBufferSize);

		// Destroy staging resources
		// stagingBuffer.CleanBuffer();
		instanceBuffer->Flush ();
		isDirty = false;
	}
}


void InstancedSceneObject::AddInstance (InstanceData data)
{

	// Log::Debug << "Adding instance at " << data.pos.x << " " << data.pos.z << "\n";

	std::lock_guard<std::mutex> lk (instanceDataLock);
	if (instanceCount < maxInstanceCount)
	{
		instancesData.at (instanceCount) = data;
		instanceCount++;
	}
	isDirty = true;
}

void InstancedSceneObject::AddInstances (std::vector<InstanceData>& newInstances)
{
	// for (auto it = positions.begin(); it != positions.end(); it++) {
	//	ModelBufferObject ubo = {};
	//	ubo.model = cml::translate(ubo.model, *it);
	//	ubo.normal = cml::mat4f();
	//	modelUniforms.push_back(ubo);
	//}

	// modelUniformsBuffer.map(renderer.device.device);
	// modelUniformsBuffer.copyTo(&modelUniforms, modelUniforms.size() * sizeof(ModelBufferObject));
	// modelUniformsBuffer.unmap();

	/*for (int i = 0; i < positions.size(); i++)
	{
	    InstanceData id;
	    id.pos = positions[i];
	    id.rot = cml::vec3f(0,0,0);
	    id.scale = 1.0f;
	    instancesData.push_back(id);
	}*/
	// for (auto& val : newInstances)
	//	Log::Debug << "Adding instance at " << val.pos.x << " " << val.pos.z << "\n";

	std::lock_guard<std::mutex> lk (instanceDataLock);
	int curFree = instanceCount;
	for (int i = 0; i < newInstances.size (); i++)
	{
		if (instanceCount < maxInstanceCount)
		{
			instancesData.at (i + curFree) = newInstances.at (i);
		}
	}
	instanceCount += newInstances.size ();
	isDirty = true;
}

void InstancedSceneObject::RemoveInstance (InstanceData instance)
{


	// Log::Debug << "Removing instance at " << instance.pos.x << " " << instance.pos.z << "\n";

	std::lock_guard<std::mutex> lk (instanceDataLock);
	auto foundInstance = std::find (std::begin (instancesData), std::end (instancesData), instance);
	if (foundInstance != std::end (instancesData))
	{
		instancesData.erase (foundInstance);
		instanceCount--;
	}
	isDirty = true;
}

void InstancedSceneObject::RemoveInstances (std::vector<InstanceData>& instances)
{

	// for (auto& val : instances)
	//	Log::Debug << "Removing instance at " << val.pos.x << " " << val.pos.z << "\n";

	std::lock_guard<std::mutex> lk (instanceDataLock);
	std::vector<std::vector<InstanceData>::iterator> indexesToDelete (instances.size ());

	for (auto& instance : instances)
	{

		auto foundInstance = std::find (std::begin (instancesData), std::end (instancesData), instance);
		if (foundInstance != std::end (instancesData))
		{
			indexesToDelete.push_back (foundInstance);
			instanceCount--;
		}
	}
	for (auto& index : indexesToDelete)
	{
		instancesData.erase (index);
	}
	isDirty = true;
}

void InstancedSceneObject::ReplaceAllInstances (std::vector<InstanceData>& instances)
{
	std::lock_guard<std::mutex> lk (instanceDataLock);
	instancesData.clear ();
	instanceCount = 0;

	if (instances.size () > 0)
	{
		for (auto& instance : instances)
			instancesData.push_back (instance);
	}
	isDirty = true;
}

void InstancedSceneObject::RemoveAllInstances ()
{
	std::lock_guard<std::mutex> lk (instanceDataLock);
	instancesData.clear ();
	instanceCount = 0;
	isDirty = true;
}

void InstancedSceneObject::UploadData ()
{
	// for (auto it = modelUniforms.begin(); it != modelUniforms.end(); it++) {
	// ModelBufferObject ubo = {};
	// ubo.model = cml::mat4f();
	////ubo.model = cml::translate(ubo.model, cml::vec3f(50, 0, 0));
	// ubo.model = cml::rotate(ubo.model, time / 2.0f, cml::vec3f(0.5, 1, 0));
	// ubo.normal = cml::transpose(cml::inverse(cml::mat3f(ubo.model)));
	//}

	// instanceBuffer->map(renderer.device.device);
	instanceBuffer->CopyToBuffer (instancesData.data (), instancesData.size () * instanceMemberSize);

	// instanceBuffer->unmap();

	UploadInstances ();
}

void InstancedSceneObject::WriteToCommandBuffer (VkCommandBuffer commandBuffer, bool wireframe)
{

	// if ((*vulkanModel->readyToUse) && (*vulkanTexture->readyToUse))
	//	isReadyToRender = true;
	//

	if (*vulkanModel->readyToUse == false || *vulkanTexture->readyToUse == false) //|| *isFinishedTransfer != true)
		return;

	VkDeviceSize offsets[] = { 0 };

	if (wireframe)
		this->wireframe->Bind (commandBuffer);
	else
		normal->Bind (commandBuffer);
	// vkCmdBindPipeline (commandBuffer,
	//    VK_PIPELINE_BIND_POINT_GRAPHICS,
	//    wireframe ? mvp->pipelines->at (1) : mvp->pipelines->at (0));
	vkCmdBindDescriptorSets (
	    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normal->GetLayout (), 2, 1, &m_descriptorSet.set, 0, nullptr);

	vulkanModel->BindModel (commandBuffer);

	instanceBuffer->BindInstanceBuffer (commandBuffer);

	// Binding point 0 : Mesh vertex buffer
	// vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, &vulkanModel->vmaVertices.buffer.buffer, offsets);
	// Binding point 1 : Instance data buffer
	// vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, &instanceBuffer->buffer.buffer, offsets);
	// vkCmdBindIndexBuffer(commandBuffer, vulkanModel->vmaIndicies.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed (commandBuffer, static_cast<uint32_t> (vulkanModel->indexCount), instanceCount, 0, 0, 0);
}


void InstancedSceneObject::ImGuiShowInstances ()
{
	bool value = true;
	if (ImGui::Begin ("instance data", &value))
	{
		if (ImGui::Button ("UploadInstance"))
		{
			// UploadInstances();
		}
		/*
		for (auto& instance : instancesData) {
		    ImGui::DragFloat3("Position", (&(instance.pos.x)));
		    ImGui::DragFloat3("Rotation", (&(instance.rot.x)));
		    ImGui::DragFloat("Scale", &instance.scale);
		    ImGui::Separator();
	}
		*/
	}
	ImGui::End ();
}