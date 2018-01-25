#pragma once

#include <vulkan/vulkan.h>

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#include "RendererStructs.h"
#include "VulkanTools.h"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include "../resources/Mesh.h"
#include "../core/CoreTools.h"


class VulkanModel {
public:

	//VulkanBuffer vertices;
	//VulkanBuffer indices;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	VulkanBufferVertex vmaVertices;
	VulkanBufferIndex vmaIndicies;

	struct ModelPart {
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};
	std::vector<ModelPart> parts;

	bool loadFromMesh(std::shared_ptr<Mesh> mesh, VulkanDevice &device, VkQueue copyQueue);

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
	bool loadFromFile(const std::string& filename, VulkanDevice &device, VkQueue copyQueue);

	void BindModel(VkCommandBuffer cmdBuf);

	/** @brief Release all Vulkan resources of this model */
	void destroy(VulkanDevice& device);
};
