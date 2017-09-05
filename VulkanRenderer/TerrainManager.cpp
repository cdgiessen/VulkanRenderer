#include "TerrainManager.h"

#include "ImGui\imgui.h"

TerrainManager::TerrainManager(VulkanDevice device) : device(device)
{
}


TerrainManager::~TerrainManager()
{
}

void TerrainManager::GenerateTerrain(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera) {
	//free resources then delete all created terrains/waters
	for (Terrain* ter : terrains) {
		ter->CleanUp();
	}
	terrains.clear();
	for (Water* water : waters) {
		water->CleanUp();
	}
	waters.clear();

	for (int i = 0; i < terrainGridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < terrainGridDimentions; j++) {
			terrains.push_back(new Terrain(100, terrainMaxLevels, (i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, terrainWidth, terrainWidth));
		}
	}

	for (Terrain* ter : terrains) {
		ter->InitTerrain(&device, renderPass, device.graphics_queue, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer, camera->Position);
	}

	for (int i = 0; i < terrainGridDimentions; i++) {
		for (int j = 0; j < terrainGridDimentions; j++) {
			waters.push_back(new Water(200, (i - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, (j - terrainGridDimentions / 2) * terrainWidth - terrainWidth / 2, terrainWidth, terrainWidth));
		}
	}

	for (Water* water : waters) {
		water->LoadTexture("Resources/Textures/TileableWaterTexture.jpg");
		water->InitWater(&device, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);
	}

	recreateTerrain = false;
}

void TerrainManager::ReInitTerrain(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain) {
	for (Terrain* ter : terrains) {
		ter->ReinitTerrain(&device, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	}
	for (Water* water : waters) {
		water->ReinitWater(&device, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	}
}

void TerrainManager::UpdateTerrains(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera, TimeManager* timeManager) {
	if (recreateTerrain) {
		GenerateTerrain(renderPass, vulkanSwapChain, globalVariableBuffer, lightsInfoBuffer, camera);
	}

	for (Water* water : waters) {
		water->UpdateUniformBuffer(timeManager->GetRunningTime(), camera->GetViewMatrix());
	}

	for (Terrain* ter : terrains) {
		terrainUpdateTimer.StartTimer();
		ter->UpdateTerrain(camera->Position, device.graphics_queue, globalVariableBuffer, lightsInfoBuffer);
		terrainUpdateTimer.EndTimer();
	}

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };
	
	for (Terrain* ter : terrains) {
		ter->DrawTerrain(commandBuffer, offsets, ter, wireframe);
	}

	//water
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? waters.at(0)->wireframe : waters.at(0)->pipeline);
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &waters.at(0)->WaterModel.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, waters.at(0)->WaterModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	for (Water* water : waters) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, water->pipelineLayout, 0, 1, &water->descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(water->WaterModel.indexCount), 1, 0, 0, 0);
	}
}

void TerrainManager::UpdateTerrainGUI() {
	
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Terrain Debug Info", &show_terrain_manager_window);
	ImGui::SliderFloat("Terrain Width", &terrainWidth, 1, 10000);
	ImGui::SliderInt("Terrain Max Subdivision", &terrainMaxLevels, 0, 10);
	ImGui::SliderInt("Terrain Grid Width", &terrainGridDimentions, 1, 10);

	if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
		recreateTerrain = true;
	}

	ImGui::Text("All terrains update Time: %u(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
	for (Terrain* ter : terrains)
	{
		ImGui::Text("Terrain Draw Time: %u(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
		ImGui::Text("Terrain Quad Count %d", ter->numQuads);
	}
	ImGui::End();
	
}

void TerrainManager::CleanUpTerrain() {

	for (Terrain* ter : terrains) {
		ter->CleanUp();
	}
	for (Water* water : waters) {
		water->CleanUp();
	}
}
