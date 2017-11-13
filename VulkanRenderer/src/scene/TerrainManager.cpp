#include "TerrainManager.h"

#include "..\third-party\ImGui\imgui.h"
#include "..\gui\ImGuiImpl.h"

TerrainManager::TerrainManager(std::shared_ptr<VulkanDevice> device) : device(device)
{
	if (terrainMaxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + 50 * terrainMaxLevels; //with current quad density this is the average upper bound
											   //maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}

	//terrainQuadPool = std::make_shared<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>>();
}


TerrainManager::~TerrainManager()
{
}

void TerrainManager::GenerateTerrain(std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer,
	VulkanBuffer lightsInfoBuffer, std::shared_ptr<Camera> camera) {
	//free resources then delete all created terrains/waters
	CleanUpTerrain();

	int numCells = 64;
	int logicalWidth = 128;// (int)numCells * glm::pow(2.0, terrainMaxLevels);
	for (int i = 0; i < terrainGridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < terrainGridDimentions; j++) {
			
			terrains.push_back(std::make_shared<Terrain>(terrainQuadPool, numCells, terrainMaxLevels, terrainHeightScale, sourceImageResolution,
				glm::vec2((i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2), //position
				glm::vec2(terrainWidth, terrainWidth), //size
				glm::i32vec2(i * logicalWidth, j * logicalWidth), //noise position
				glm::i32vec2(logicalWidth, logicalWidth))); //noiseSize 
		}
	}

	//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	for (auto ter : terrains) {
		ter->InitTerrain(device, pipelineManager, renderPass, device->transfer_queue, vulkanSwapChain.swapChainExtent.width, 
			vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer, camera->Position);
	}
	//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);
	
	for (int i = 0; i < terrainGridDimentions; i++) {
		for (int j = 0; j < terrainGridDimentions; j++) {
			waters.push_back(std::make_shared< Water>
				(64, (i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, terrainWidth, terrainWidth));
		}
	}

	for (auto water : waters) {
		water->LoadTexture("Resources/Textures/TileableWaterTexture.jpg");
		water->InitWater(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);
	}

	recreateTerrain = false;
}

void TerrainManager::ReInitTerrain(std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain) {
	for (auto ter : terrains) {
		ter->ReinitTerrain(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	}
	for (auto water : waters) {
		water->ReinitWater(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	}
}

void TerrainManager::UpdateTerrains(std::shared_ptr<VulkanPipeline> pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer,
	VulkanBuffer lightsInfoBuffer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager) {
	if (recreateTerrain) {

		GenerateTerrain(pipelineManager, renderPass, vulkanSwapChain, globalVariableBuffer, lightsInfoBuffer, camera);
	}

	for (auto water : waters) {
		water->UpdateUniformBuffer((float)timeManager->GetRunningTime(), camera->GetViewMatrix());
	}

	for (auto ter : terrains) {
		terrainUpdateTimer.StartTimer();
		//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		ter->UpdateTerrain(camera->Position, device->transfer_queue, globalVariableBuffer, lightsInfoBuffer);

		//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);
		terrainUpdateTimer.EndTimer();
		//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
		//	std::cout << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << std::endl;
		//}
	}

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };
	
	for (auto ter : terrains) {
		ter->DrawTerrain(commandBuffer, offsets, ter, wireframe);
	}

	//water
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? waters.at(0)->wireframe : waters.at(0)->pipeline);
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? waters.at(0)->wireframe : waters.at(0)->seascapePipeline);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &waters.at(0)->WaterModel.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, waters.at(0)->WaterModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	for (auto water : waters) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, water->pipelineLayout, 0, 1, &water->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(water->WaterModel.indexCount), 1, 0, 0, 0);
	}
}

void TerrainManager::UpdateTerrainGUI() {
	
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Debug Info", &show_terrain_manager_window);
	ImGui::SliderFloat("Width", &terrainWidth, 1, 10000);
	ImGui::SliderInt("Max Subdivision", &terrainMaxLevels, 0, 10);
	ImGui::SliderInt("Grid Width", &terrainGridDimentions, 1, 10);
	ImGui::SliderFloat("Height Scale", &terrainHeightScale, 1, 1000);
	ImGui::SliderInt("Image Resolution", &sourceImageResolution, 1, 2048);

	if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
		recreateTerrain = true;
	}

	ImGui::Text("All terrains update Time: %u(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
	for (auto ter : terrains)
	{
		ImGui::Text("Terrain Draw Time: %u(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
		ImGui::Text("Terrain Quad Count %d", ter->numQuads);
	}
	ImGui::End();
	
}

void TerrainManager::CleanUpTerrain() {

	terrains.clear();
	waters.clear();
	//delete terrainQuadPool;
	//terrainQuadPool = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
}

/**
* Allocate a command buffer from the command pool
*
* @param level Level of the new command buffer (primary or secondary)
* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
*
* @return A handle to the allocated command buffer
*/
VkCommandBuffer TerrainManager::CreateTerrainMeshUpdateCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = initializers::commandBufferAllocateInfo(commandPool, level, (uint32_t)terrains.size());

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device->device, &cmdBufAllocateInfo, &cmdBuffer));

	
	VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();
	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	return cmdBuffer;
}

/**
* Finish command buffer recording and submit it to a queue
*
* @param commandBuffer Command buffer to flush
* @param queue Queue to submit the command buffer to
* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
*
* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
* @note Uses a fence to ensure command buffer has finished executing
*/
void TerrainManager::FlushTerrainMeshUpdateCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}


	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(device->device, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(device->device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(device->device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(device->device, device->graphics_queue_command_pool, 1, &commandBuffer);
	}
}
