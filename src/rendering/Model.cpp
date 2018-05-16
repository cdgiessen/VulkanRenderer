#include "Model.h"

#include "Renderer.h"

VulkanModel::VulkanModel(VulkanDevice &device): device(device),
vmaVertices(device), vmaIndicies(device)
{

}

bool VulkanModel::loadFromMesh(std::shared_ptr<Mesh> mesh, 
	VulkanRenderer& renderer) {

	std::vector<float> vertexBuffer;
	std::vector<uint32_t> indexBuffer;

	indexCount = static_cast<uint32_t>(mesh->indices.size());
	vertexElementCount = mesh->vertexElementCount;
	vertexBuffer.resize(vertexCount * vertexElementCount);

	if(vertexElementCount == 6){
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNorm>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
			{
				vertexBuffer[i * 6 + 0] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[0];
				vertexBuffer[i * 6 + 1] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[1];
				vertexBuffer[i * 6 + 2] = std::get<Vertices_PosNorm>(mesh->vertices)[i].pos[2];
				vertexBuffer[i * 6 + 3] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[0];
				vertexBuffer[i * 6 + 4] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[1];
				vertexBuffer[i * 6 + 5] = std::get<Vertices_PosNorm>(mesh->vertices)[i].normal[2];
			}
		} 
	else if (vertexElementCount == 8){
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTex>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * mesh->vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
			{
				vertexBuffer[i * 8 + 0] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[0];
				vertexBuffer[i * 8 + 1] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[1];
				vertexBuffer[i * 8 + 2] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].pos[2];
				vertexBuffer[i * 8 + 3] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[0];
				vertexBuffer[i * 8 + 4] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[1];
				vertexBuffer[i * 8 + 5] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].normal[2];
				vertexBuffer[i * 8 + 6] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[0];
				vertexBuffer[i * 8 + 7] = std::get<Vertices_PosNormTex>(mesh->vertices)[i].texCoord[1];
			}
	
	} else {
		vertexCount = static_cast<uint32_t>(std::get<Vertices_PosNormTexColor>(mesh->vertices).size());
		vertexBuffer.resize(vertexCount * vertexElementCount);

		for (int i = 0; i < (int)vertexCount; i++)
		{
			vertexBuffer[i * 12 + 0] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[0];
			vertexBuffer[i * 12 + 1] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[1];
			vertexBuffer[i * 12 + 2] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).pos[2];
			vertexBuffer[i * 12 + 3] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[0];
			vertexBuffer[i * 12 + 4] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[1];
			vertexBuffer[i * 12 + 5] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).normal[2];
			vertexBuffer[i * 12 + 6] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[0];
			vertexBuffer[i * 12 + 7] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).texCoord[1];
			vertexBuffer[i * 12 + 8] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[0];
			vertexBuffer[i * 12 + 9] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[1];
			vertexBuffer[i * 12 + 10] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[2];
			vertexBuffer[i * 12 + 11] = (std::get<Vertices_PosNormTexColor>(mesh->vertices)).at(i).color[3];
		
		
        }
	}

	indexBuffer.resize(indexCount);
	for (int i = 0; i < (int)indexCount; i++)
	{
		indexBuffer[i] = mesh->indices[i];
	}

	uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
	uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);


	vmaVertices.CreateVertexBuffer((uint32_t)vertexBuffer.size(), vertexElementCount);
	vmaIndicies.CreateIndexBuffer((uint32_t)indexBuffer.size());

	VulkanBufferVertex vertexStagingBuffer(device);
	VulkanBufferIndex indexStagingBuffer(device);

	vertexStagingBuffer.CreateStagingVertexBuffer(vertexBuffer.data(), (uint32_t)vertexCount, vertexElementCount);
	indexStagingBuffer.CreateStagingIndexBuffer(indexBuffer.data(), (uint32_t)indexCount);

	VkBufferCopy copyRegion{};

	VkCommandBuffer copyCmd = renderer.GetTransferCommandBuffer();

	copyRegion.size = vBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStagingBuffer.buffer.buffer, vmaVertices.buffer.buffer, 1, &copyRegion);

	copyRegion.size = iBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStagingBuffer.buffer.buffer, vmaIndicies.buffer.buffer, 1, &copyRegion);

	renderer.SubmitTransferCommandBufferAndWait(copyCmd);

	vertexStagingBuffer.CleanBuffer();
	indexStagingBuffer.CleanBuffer();

	return true;



	//// Use staging buffer to move vertex and index buffer to device local memory
	//// Create staging buffers
	//VulkanBuffer vertexStaging, indexStaging;

	//// Vertex buffer
	//VK_CHECK_RESULT(device.createBuffer(
	//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	//	&vertexStaging,
	//	vBufferSize,
	//	vertexBuffer.data()));
	////auto vbdata = vertexBuffer.data();
	//// Index buffer
	//VK_CHECK_RESULT(device.createBuffer(
	//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	//	&indexStaging,
	//	iBufferSize,
	//	indexBuffer.data()));

	//// Create device local target buffers
	//// Vertex buffer
	//VK_CHECK_RESULT(device.createBuffer(
	//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	&vertices,
	//	vBufferSize));

	//// Index buffer
	//VK_CHECK_RESULT(device.createBuffer(
	//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	&indices,
	//	iBufferSize));

	//// Copy from staging buffers
	////VkCommandBuffer copyCmd = 
	//VkCommandBuffer copyCmd = device.createCommandBuffer(device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//VkBufferCopy copyRegion{};

	//copyRegion.size = vertices.size;
	//vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

	//copyRegion.size = indices.size;
	//vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

	//device.flushCommandBuffer(copyCmd, copyQueue);

	//// Destroy staging resources
	//vkDestroyBuffer(device.device, vertexStaging.buffer, nullptr);
	//vkFreeMemory(device.device, vertexStaging.bufferMemory, nullptr);
	//vkDestroyBuffer(device.device, indexStaging.buffer, nullptr);
	//vkFreeMemory(device.device, indexStaging.bufferMemory, nullptr);

	//return true;
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
bool VulkanModel::loadFromFile(const std::string& filename, VkQueue copyQueue)
{
	//Mesh importedMesh;
	//if (fileExists(filename)) { //file exists and can be loaded
	//	importedMesh.importFromFile(filename);

	//	std::vector<float> vertexBuffer(importedMesh.vertexCount * 12);
	//	std::vector<uint32_t> indexBuffer(importedMesh.indexCount);

	//	for (int i = 0; i < importedMesh.vertices.size(); i++) {
	//		vertexBuffer[i * 12 + 0] = importedMesh.vertices[i].pos.x;
	//		vertexBuffer[i * 12 + 1] = importedMesh.vertices[i].pos.y;
	//		vertexBuffer[i * 12 + 2] = importedMesh.vertices[i].pos.z;
	//		vertexBuffer[i * 12 + 3] = importedMesh.vertices[i].normal.x;
	//		vertexBuffer[i * 12 + 4] = importedMesh.vertices[i].normal.y;
	//		vertexBuffer[i * 12 + 5] = importedMesh.vertices[i].normal.z;
	//		vertexBuffer[i * 12 + 6] = importedMesh.vertices[i].texCoord.x;
	//		vertexBuffer[i * 12 + 7] = importedMesh.vertices[i].texCoord.y;
	//		vertexBuffer[i * 12 + 8] = importedMesh.vertices[i].color.r;
	//		vertexBuffer[i * 12 + 9] = importedMesh.vertices[i].color.g;
	//		vertexBuffer[i * 12 + 10] = importedMesh.vertices[i].color.b;
	//		vertexBuffer[i * 12 + 11] = importedMesh.vertices[i].color.a;
	//	}

	//	for (int i = 0; i < importedMesh.indices.size(); i++) {
	//		indexBuffer[i] = importedMesh.indices[i];
	//	}

	//Assimp::Importer Importer;
	//const aiScene* pScene;

	//pScene = Importer.ReadFile(filename.c_str(), aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);

	//if (pScene)
	//{
	//	parts.clear();
	//	parts.resize(pScene->mNumMeshes);


	//	std::vector<float> vertexBuffer;
	//	std::vector<uint32_t> indexBuffer;


	//	// Load meshes
	//	for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
	//	{
	//		const aiMesh* paiMesh = pScene->mMeshes[i];

	//		parts[i] = {};
	//		parts[i].vertexBase = vertexCount;
	//		parts[i].indexBase = indexCount;

	//		vertexCount += pScene->mMeshes[i]->mNumVertices;

	//		aiColor3D pColor(0.f, 0.f, 0.f);
	//		pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

	//		const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	//		for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
	//		{
	//			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
	//			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
	//			const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
	//			const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
	//			const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

	//			vertexBuffer.push_back(pPos->x * scale.x + center.x);
	//			vertexBuffer.push_back(-pPos->y * scale.y + center.y);
	//			vertexBuffer.push_back(pPos->z * scale.z + center.z);

	//			vertexBuffer.push_back(pNormal->x);
	//			vertexBuffer.push_back(-pNormal->y);
	//			vertexBuffer.push_back(pNormal->z);

	//			vertexBuffer.push_back(pTexCoord->x * uvscale.s);
	//			vertexBuffer.push_back(pTexCoord->y * uvscale.t);

	//			vertexBuffer.push_back(pColor.r);
	//			vertexBuffer.push_back(pColor.g);
	//			vertexBuffer.push_back(pColor.b);
	//			vertexBuffer.push_back(1.0f);

	//			/*for (auto& component : layout.components)
	//			{
	//			switch (component) {
	//			case VERTEX_COMPONENT_POSITION:
	//			vertexBuffer.push_back(pPos->x * scale.x + center.x);
	//			vertexBuffer.push_back(-pPos->y * scale.y + center.y);
	//			vertexBuffer.push_back(pPos->z * scale.z + center.z);
	//			break;
	//			case VERTEX_COMPONENT_NORMAL:
	//			vertexBuffer.push_back(pNormal->x);
	//			vertexBuffer.push_back(-pNormal->y);
	//			vertexBuffer.push_back(pNormal->z);
	//			break;
	//			case VERTEX_COMPONENT_UV:
	//			vertexBuffer.push_back(pTexCoord->x * uvscale.s);
	//			vertexBuffer.push_back(pTexCoord->y * uvscale.t);
	//			break;
	//			case VERTEX_COMPONENT_COLOR:
	//			vertexBuffer.push_back(pColor.r);
	//			vertexBuffer.push_back(pColor.g);
	//			vertexBuffer.push_back(pColor.b);
	//			break;
	//			case VERTEX_COMPONENT_TANGENT:
	//			vertexBuffer.push_back(pTangent->x);
	//			vertexBuffer.push_back(pTangent->y);
	//			vertexBuffer.push_back(pTangent->z);
	//			break;
	//			case VERTEX_COMPONENT_BITANGENT:
	//			vertexBuffer.push_back(pBiTangent->x);
	//			vertexBuffer.push_back(pBiTangent->y);
	//			vertexBuffer.push_back(pBiTangent->z);
	//			break;
	//			// Dummy components for padding
	//			case VERTEX_COMPONENT_DUMMY_FLOAT:
	//			vertexBuffer.push_back(0.0f);
	//			break;
	//			case VERTEX_COMPONENT_DUMMY_VEC4:
	//			vertexBuffer.push_back(0.0f);
	//			vertexBuffer.push_back(0.0f);
	//			vertexBuffer.push_back(0.0f);
	//			vertexBuffer.push_back(0.0f);
	//			break;
	//			};
	//			}*/
	//		}

	//		parts[i].vertexCount = paiMesh->mNumVertices;

	//		uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
	//		for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
	//		{
	//			const aiFace& Face = paiMesh->mFaces[j];
	//			if (Face.mNumIndices != 3)
	//				continue;
	//			indexBuffer.push_back(indexBase + Face.mIndices[0]);
	//			indexBuffer.push_back(indexBase + Face.mIndices[1]);
	//			indexBuffer.push_back(indexBase + Face.mIndices[2]);
	//			parts[i].indexCount += 3;
	//			indexCount += 3;
	//		}
	//	}

	//	uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
	//	uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

	//	// Use staging buffer to move vertex and index buffer to device local memory
	//	// Create staging buffers
	//	VulkanBuffer vertexStaging, indexStaging;

	//	// Vertex buffer
	//	VK_CHECK_RESULT(device.createBuffer(
	//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	//		&vertexStaging,
	//		vBufferSize,
	//		vertexBuffer.data()));

	//	// Index buffer
	//	VK_CHECK_RESULT(device.createBuffer(
	//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	//		&indexStaging,
	//		iBufferSize,
	//		indexBuffer.data()));

	//	// Create device local target buffers
	//	// Vertex buffer
	//	VK_CHECK_RESULT(device.createBuffer(
	//		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//		&vertices,
	//		vBufferSize));

	//	// Index buffer
	//	VK_CHECK_RESULT(device.createBuffer(
	//		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//		&indices,
	//		iBufferSize));

	//	// Copy from staging buffers
	//	VkCommandBuffer copyCmd = device.createCommandBuffer(device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//	VkBufferCopy copyRegion{};

	//	copyRegion.size = vertices.size;
	//	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

	//	copyRegion.size = indices.size;
	//	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

	//	device.flushCommandBuffer(copyCmd, copyQueue);

	//	// Destroy staging resources
	//	vkDestroyBuffer(device.device, vertexStaging.buffer, nullptr);
	//	vkFreeMemory(device.device, vertexStaging.bufferMemory, nullptr);
	//	vkDestroyBuffer(device.device, indexStaging.buffer, nullptr);
	//	vkFreeMemory(device.device, indexStaging.bufferMemory, nullptr);

	//	return true;
	//}
	//else
	//{
	//	printf("Error parsing '%s': '\n", filename.c_str());
	//	return false;
	//}
	//

	return true;
};

void VulkanModel::BindModel(VkCommandBuffer cmdBuf) {
	vmaVertices.BindVertexBuffer(cmdBuf);
	vmaIndicies.BindIndexBuffer(cmdBuf);
}

/** @brief Release all Vulkan resources of this model */
void VulkanModel::destroy()
{
	vmaVertices.CleanBuffer();
	vmaIndicies.CleanBuffer();
}