#include "TerrainManager.h"

#include "../../third-party/ImGui/imgui.h"
#include "../gui/ImGuiImpl.h"

#include "../core/Logger.h"

TerrainManager::TerrainManager(NewNodeGraph::TerGenNodeGraph& nodeGraph): nodeGraph(nodeGraph)
{
	if (terrainMaxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + 50 * terrainMaxLevels; //with current quad density this is the average upper bound
											   //maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}

	//terrainQuadPool = std::make_shared<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>>();
	//nodeGraph.BuildNoiseGraph();




}

void TerrainManager::SetupResources(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer) {
	terrainTextureArray = resourceMan->texManager.loadTextureArrayFromFile("assets/Textures/TerrainTextures/", terrainTextureFileNames);
	terrainVulkanTextureArray.loadTextureArray(renderer->device, terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);
	
	WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
	WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);

}

void TerrainManager::CleanUpResources() {
	terrainVulkanTextureArray.destroy(renderer->device);
	WaterVulkanTexture.destroy(renderer->device);

	WaterModel.destroy(renderer->device);
}


TerrainManager::~TerrainManager()
{
	Log::Debug << "terrain manager deleted\n";
}

void TerrainManager::GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera) {
	this->renderer = renderer;

	for (int i = 0; i < terrainGridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < terrainGridDimentions; j++) {
			
			auto terrain = std::make_shared<Terrain>(terrainQuadPool, nodeGraph, numCells, terrainMaxLevels, terrainHeightScale, sourceImageResolution,
				glm::vec2((i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2), //position
				glm::vec2(terrainWidth, terrainWidth), //size
				glm::i32vec2(i * logicalWidth, j * logicalWidth), //noise position
				glm::i32vec2(logicalWidth, logicalWidth)); //noiseSize 

			//terrain->terrainTextureArray = resourceMan->texManager.loadTextureArrayFromFile("assets/Textures/TerrainTextures/", terrainTextureFileNames);
			terrain->terrainSplatMap = resourceMan->texManager.loadTextureFromRGBAPixelData(sourceImageResolution, sourceImageResolution, terrain->LoadSplatMapFromGenerator());

			terrains.push_back(terrain);
		}
	}

	//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	for (auto ter : terrains) {
		ter->InitTerrain(renderer, camera->Position, &terrainVulkanTextureArray);
	}
	//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);
	
	WaterMesh = createFlatPlane(numCells, glm::vec3(terrainWidth, 0, terrainWidth));
	WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);


	for (int i = 0; i < terrainGridDimentions; i++) {
		for (int j = 0; j < terrainGridDimentions; j++) {
			auto water = std::make_shared< Water>
				(64, (i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, terrainWidth, terrainWidth);
			
			//WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
			water->InitWater(renderer, WaterVulkanTexture);
			
			waters.push_back(water);
		}
	}

	recreateTerrain = false;
}

void TerrainManager::UpdateTerrains(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager) {
	this->renderer = renderer;
	if (recreateTerrain) {
		CleanUpTerrain();
		GenerateTerrain(resourceMan, renderer, camera);
	}

	for (auto ter : terrains) {
		terrainUpdateTimer.StartTimer();
		//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		ter->UpdateTerrain(camera->Position);

		//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);
		terrainUpdateTimer.EndTimer();
		//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
		//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
		//}
	}

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };
	
	for (auto ter : terrains) {
		ter->DrawTerrain(commandBuffer, offsets, ter, wireframe);
	}

	//water
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? waters.at(0)->mvp->pipelines->at(1) : waters.at(0)->mvp->pipelines->at(0));
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? waters.at(0)->wireframe : waters.at(0)->seascapePipeline);
	
	WaterModel.BindModel(commandBuffer);
	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &waters.at(0)->WaterModel.vmaBufferVertex, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, waters.at(0)->WaterModel.vmaBufferIndex, 0, VK_INDEX_TYPE_UINT32);

	for (auto water : waters) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, water->mvp->layout, 2, 1, &water->m_descriptorSet.set, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(WaterModel.indexCount), 1, 0, 0, 0);
	}
}

float TerrainManager::GetTerrainHeightAtLocation(float x, float z) {
	for (auto terrain : terrains)
	{
		glm::vec2 pos = terrain->position;
		glm::vec2 size = terrain->size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z) {
			return terrain->GetHeightAtLocation((x - pos.x)/ terrainWidth, (z - pos.y)/ terrainWidth);
		}
	}
	return 0;
}

void TerrainManager::UpdateTerrainGUI() {
	
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Debug Info", &show_terrain_manager_window);
	ImGui::SliderFloat("Width", &terrainWidth, 100, 10000);
	ImGui::SliderInt("Max Subdivision", &terrainMaxLevels, 0, 10);
	ImGui::SliderInt("Grid Width", &terrainGridDimentions, 1, 10);
	ImGui::SliderFloat("Height Scale", &terrainHeightScale, 1, 1000);
	ImGui::SliderInt("Image Resolution", &sourceImageResolution, 32, 2048);

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

void TerrainManager::RecreateTerrain(){
	recreateTerrain = true;
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
	VK_CHECK_RESULT(vkAllocateCommandBuffers(renderer->device.device, &cmdBufAllocateInfo, &cmdBuffer));

	
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
	VK_CHECK_RESULT(vkCreateFence(renderer->device.device, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(renderer->device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(renderer->device.device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(renderer->device.device, renderer->device.graphics_queue_command_pool, 1, &commandBuffer);
	}
}
