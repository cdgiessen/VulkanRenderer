#pragma once

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#include "vulkan\vulkan.h"
#include "VulkanTools.h"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "..\core\Mesh.h"

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

	bool loadFromMesh(Mesh* mesh, VulkanDevice* device, VkQueue copyQueue) {

		this->device = device->device;

		std::vector<float> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		vertexCount = static_cast<uint32_t>(mesh->vertices.size());
		indexCount = static_cast<uint32_t>(mesh->indices.size());

		vertexBuffer.resize(vertexCount * 12);
		for (int i = 0; i < (int)vertexCount; i++)
		{
			vertexBuffer[i * 12]	 = mesh->vertices[i].pos[0];
			vertexBuffer[i * 12 + 1] = mesh->vertices[i].pos[1];
			vertexBuffer[i * 12 + 2] = mesh->vertices[i].pos[2];
			vertexBuffer[i * 12 + 3] = mesh->vertices[i].normal[0];
			vertexBuffer[i * 12 + 4] = mesh->vertices[i].normal[1];
			vertexBuffer[i * 12 + 5] = mesh->vertices[i].normal[2];
			vertexBuffer[i * 12 + 6] = mesh->vertices[i].texCoord[0];
			vertexBuffer[i * 12 + 7] = mesh->vertices[i].texCoord[1];
			vertexBuffer[i * 12 + 8] = mesh->vertices[i].color[0];
			vertexBuffer[i * 12 + 9] = mesh->vertices[i].color[1];
			vertexBuffer[i * 12 + 10] = mesh->vertices[i].color[2];
			vertexBuffer[i * 12 + 11] = mesh->vertices[i].color[3];
		}

		indexBuffer.resize(indexCount);
		for (int i = 0; i < (int)indexCount; i++)
		{
			indexBuffer[i] = mesh->indices[i];
		}

		

		uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
		uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

		// Use staging buffer to move vertex and index buffer to device local memory
		// Create staging buffers
		VulkanBuffer vertexStaging, indexStaging;

		// Vertex buffer
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			&vertexStaging,
			vBufferSize,
			vertexBuffer.data()));
		//auto vbdata = vertexBuffer.data();
		// Index buffer
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			&indexStaging,
			iBufferSize,
			indexBuffer.data()));

		// Create device local target buffers
		// Vertex buffer
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertices,
			vBufferSize));

		// Index buffer
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indices,
			iBufferSize));

		// Copy from staging buffers
		VkCommandBuffer copyCmd = device->createCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion{};

		copyRegion.size = vertices.size;
		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

		copyRegion.size = indices.size;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

		device->flushCommandBuffer(copyCmd, copyQueue);

		// Destroy staging resources
		vkDestroyBuffer(device->device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device->device, vertexStaging.bufferMemory, nullptr);
		vkDestroyBuffer(device->device, indexStaging.buffer, nullptr);
		vkFreeMemory(device->device, indexStaging.bufferMemory, nullptr);

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
	bool loadFromFile(const std::string& filename, VulkanDevice* device, VkQueue copyQueue)
	{
		this->device = device->device;

		if (true) { //file exists and can be loaded

			std::vector<float> vertexBuffer;
			std::vector<uint32_t> indexBuffer;

			vertexCount = 4;
			indexCount = 6;

			Assimp::Importer Importer;
			const aiScene* pScene;

			pScene = Importer.ReadFile(filename.c_str(), aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);

			if (pScene)
			{
				parts.clear();
				parts.resize(pScene->mNumMeshes);

				glm::vec3 scale(1.0f);
				glm::vec2 uvscale(1.0f);
				glm::vec3 center(0.0f);

				std::vector<float> vertexBuffer;
				std::vector<uint32_t> indexBuffer;

				vertexCount = 0;
				indexCount = 0;

				// Load meshes
				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];

					parts[i] = {};
					parts[i].vertexBase = vertexCount;
					parts[i].indexBase = indexCount;

					vertexCount += pScene->mMeshes[i]->mNumVertices;

					aiColor3D pColor(0.f, 0.f, 0.f);
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

					const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

					for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
					{
						const aiVector3D* pPos = &(paiMesh->mVertices[j]);
						const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
						const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
						const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
						const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

						vertexBuffer.push_back(pPos->x * scale.x + center.x);
						vertexBuffer.push_back(-pPos->y * scale.y + center.y);
						vertexBuffer.push_back(pPos->z * scale.z + center.z);

						vertexBuffer.push_back(pNormal->x);
						vertexBuffer.push_back(-pNormal->y);
						vertexBuffer.push_back(pNormal->z);

						vertexBuffer.push_back(pTexCoord->x * uvscale.s);
						vertexBuffer.push_back(pTexCoord->y * uvscale.t);

						vertexBuffer.push_back(pColor.r);
						vertexBuffer.push_back(pColor.g);
						vertexBuffer.push_back(pColor.b);
						vertexBuffer.push_back(1.0f);

						/*for (auto& component : layout.components)
						{
						switch (component) {
						case VERTEX_COMPONENT_POSITION:
						vertexBuffer.push_back(pPos->x * scale.x + center.x);
						vertexBuffer.push_back(-pPos->y * scale.y + center.y);
						vertexBuffer.push_back(pPos->z * scale.z + center.z);
						break;
						case VERTEX_COMPONENT_NORMAL:
						vertexBuffer.push_back(pNormal->x);
						vertexBuffer.push_back(-pNormal->y);
						vertexBuffer.push_back(pNormal->z);
						break;
						case VERTEX_COMPONENT_UV:
						vertexBuffer.push_back(pTexCoord->x * uvscale.s);
						vertexBuffer.push_back(pTexCoord->y * uvscale.t);
						break;
						case VERTEX_COMPONENT_COLOR:
						vertexBuffer.push_back(pColor.r);
						vertexBuffer.push_back(pColor.g);
						vertexBuffer.push_back(pColor.b);
						break;
						case VERTEX_COMPONENT_TANGENT:
						vertexBuffer.push_back(pTangent->x);
						vertexBuffer.push_back(pTangent->y);
						vertexBuffer.push_back(pTangent->z);
						break;
						case VERTEX_COMPONENT_BITANGENT:
						vertexBuffer.push_back(pBiTangent->x);
						vertexBuffer.push_back(pBiTangent->y);
						vertexBuffer.push_back(pBiTangent->z);
						break;
						// Dummy components for padding
						case VERTEX_COMPONENT_DUMMY_FLOAT:
						vertexBuffer.push_back(0.0f);
						break;
						case VERTEX_COMPONENT_DUMMY_VEC4:
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						break;
						};
						}*/
					}

					parts[i].vertexCount = paiMesh->mNumVertices;

					uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
					for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
					{
						const aiFace& Face = paiMesh->mFaces[j];
						if (Face.mNumIndices != 3)
							continue;
						indexBuffer.push_back(indexBase + Face.mIndices[0]);
						indexBuffer.push_back(indexBase + Face.mIndices[1]);
						indexBuffer.push_back(indexBase + Face.mIndices[2]);
						parts[i].indexCount += 3;
						indexCount += 3;
					}
				}

				uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
				uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

				// Use staging buffer to move vertex and index buffer to device local memory
				// Create staging buffers
				VulkanBuffer vertexStaging, indexStaging;

				// Vertex buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					&vertexStaging,
					vBufferSize,
					vertexBuffer.data()));

				// Index buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					&indexStaging,
					iBufferSize,
					indexBuffer.data()));

				// Create device local target buffers
				// Vertex buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&vertices,
					vBufferSize));

				// Index buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&indices,
					iBufferSize));

				// Copy from staging buffers
				VkCommandBuffer copyCmd = device->createCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

				VkBufferCopy copyRegion{};

				copyRegion.size = vertices.size;
				vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

				copyRegion.size = indices.size;
				vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

				device->flushCommandBuffer(copyCmd, copyQueue);

				// Destroy staging resources
				vkDestroyBuffer(device->device, vertexStaging.buffer, nullptr);
				vkFreeMemory(device->device, vertexStaging.bufferMemory, nullptr);
				vkDestroyBuffer(device->device, indexStaging.buffer, nullptr);
				vkFreeMemory(device->device, indexStaging.bufferMemory, nullptr);

				return true;
			}
			else
			{
				printf("Error parsing '%s': '\n", filename.c_str());
				return false;
			}

		}
	};

	/** @brief Release all Vulkan resources of this model */
	void destroy()
	{
		//assert(device);
		if (vertices.buffer != VK_NULL_HANDLE) 
		{
			vkDestroyBuffer(device, vertices.buffer, nullptr);
			vkFreeMemory(device, vertices.bufferMemory, nullptr);
		}
		if (indices.buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, indices.buffer, nullptr);
			vkFreeMemory(device, indices.bufferMemory, nullptr);
		}
	}
};