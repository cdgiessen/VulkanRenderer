#pragma once

#include "vulkan\vulkan.h"
#include "VulkanTools.h"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include "Mesh.h"

class VulkanModel {
public:



	VkDevice device;
	VulkanBuffer vertices;
	VulkanBuffer indices;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	/** @brief Stores vertex and index base and counts for each part of a model */ //multi part models...
	struct ModelPart {
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};
	std::vector<ModelPart> parts;

	bool loadFromMesh(Mesh* mesh, VulkanDevice device, VkQueue copyQueue) {

		this->device = device.device;

		std::vector<float> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		vertexCount = mesh->vertices.size();
		indexCount = mesh->indices.size();

		vertexBuffer.resize(vertexCount * 8);
		for (int i = 0; i < vertexCount; i++)
		{
			vertexBuffer[i * 8]		= mesh->vertices[i].pos[0];
			vertexBuffer[i * 8 + 1] = mesh->vertices[i].pos[1];
			vertexBuffer[i * 8 + 2] = mesh->vertices[i].pos[2];
			vertexBuffer[i * 8 + 3] = mesh->vertices[i].color[0];
			vertexBuffer[i * 8 + 4] = mesh->vertices[i].color[1];
			vertexBuffer[i * 8 + 5] = mesh->vertices[i].color[2];
			vertexBuffer[i * 8 + 6] = mesh->vertices[i].texCoord[0];
			vertexBuffer[i * 8 + 7] = mesh->vertices[i].texCoord[1];
		}

		indexBuffer.resize(indexCount);
		for (int i = 0; i < indexCount; i++)
		{
			indexBuffer[i] = mesh->indices[i];
		}

		

		uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
		uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

		// Use staging buffer to move vertex and index buffer to device local memory
		// Create staging buffers
		VulkanBuffer vertexStaging, indexStaging;

		// Vertex buffer
		VK_CHECK_RESULT(device.createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			&vertexStaging,
			vBufferSize,
			vertexBuffer.data()));

		// Index buffer
		VK_CHECK_RESULT(device.createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			&indexStaging,
			iBufferSize,
			indexBuffer.data()));

		// Create device local target buffers
		// Vertex buffer
		VK_CHECK_RESULT(device.createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertices,
			vBufferSize));

		// Index buffer
		VK_CHECK_RESULT(device.createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indices,
			iBufferSize));

		// Copy from staging buffers
		VkCommandBuffer copyCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion{};

		copyRegion.size = vertices.size;
		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

		copyRegion.size = indices.size;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

		device.flushCommandBuffer(copyCmd, copyQueue);

		// Destroy staging resources
		vkDestroyBuffer(device.device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device.device, vertexStaging.bufferMemory, nullptr);
		vkDestroyBuffer(device.device, indexStaging.buffer, nullptr);
		vkFreeMemory(device.device, indexStaging.bufferMemory, nullptr);

		return true;
	}

	/**
	* Loads a 3D model from a file into Vulkan buffers
	*
	* @param device Pointer to the Vulkan device used to generated the vertex and index buffers on
	* @param filename File to load (must be a model format supported by ASSIMP)
	* @param layout Vertex layout components (position, normals, tangents, etc.)
	* @param createInfo MeshCreateInfo structure for load time settings like scale, center, etc.
	* @param copyQueue Queue used for the memory staging copy commands (must support transfer)
	* @param (Optional) flags ASSIMP model loading flags
	*/
	bool loadFromFile(const std::string& filename, VulkanDevice device, VkQueue copyQueue)
	{
		this->device = device.device;

		if (true) { //file exists and can be loaded

			std::vector<float> vertexBuffer;
			std::vector<uint32_t> indexBuffer;

			vertexCount = 4;
			indexCount = 6;

			vertexBuffer = {  -0.5f, -0.5f, 0.0f , 1.0f, 0.0f, 0.0f , 0.0f, 0.0f ,
							   0.5f, -0.5f, 0.0f  ,  0.0f, 1.0f, 0.0f  ,  1.0f, 0.0f    ,
							   0.5f, 0.5f, 0.0f  ,  0.0f, 0.0f, 1.0f  ,  1.0f, 1.0f    ,
							   -0.5f, 0.5f, 0.0f  ,  1.0f, 1.0f, 1.0f  ,  0.0f, 1.0f   };

			indexBuffer = { 0, 1, 2, 2, 3, 0 };

			/** @todo NEEDS TO LOAD FROM FILE OR SOMEWHERE!!! not make a flat plane */

			uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
			uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

			// Use staging buffer to move vertex and index buffer to device local memory
			// Create staging buffers
			VulkanBuffer vertexStaging, indexStaging;

			// Vertex buffer
			VK_CHECK_RESULT(device.createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				&vertexStaging,
				vBufferSize,
				vertexBuffer.data()));

			// Index buffer
			VK_CHECK_RESULT(device.createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				&indexStaging,
				iBufferSize,
				indexBuffer.data()));

			// Create device local target buffers
			// Vertex buffer
			VK_CHECK_RESULT(device.createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&vertices,
				vBufferSize));

			// Index buffer
			VK_CHECK_RESULT(device.createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indices,
				iBufferSize));

			// Copy from staging buffers
			VkCommandBuffer copyCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion{};

			copyRegion.size = vertices.size;
			vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

			copyRegion.size = indices.size;
			vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

			device.flushCommandBuffer(copyCmd, copyQueue);

			// Destroy staging resources
			vkDestroyBuffer(device.device, vertexStaging.buffer, nullptr);
			vkFreeMemory(device.device, vertexStaging.bufferMemory, nullptr);
			vkDestroyBuffer(device.device, indexStaging.buffer, nullptr);
			vkFreeMemory(device.device, indexStaging.bufferMemory, nullptr);

			return true;
		}
		else
		{
			printf("Error parsing '%s': '\n", filename.c_str());
			return false;
		}

	}

	/** @brief Release all Vulkan resources of this model */
	void destroy()
	{
		//assert(device);
		vkDestroyBuffer(device, vertices.buffer, nullptr);
		vkFreeMemory(device, vertices.bufferMemory, nullptr);
		if (indices.buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, indices.buffer, nullptr);
			vkFreeMemory(device, indices.bufferMemory, nullptr);
		}
	}
};