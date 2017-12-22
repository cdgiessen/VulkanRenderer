#include "Mesh.h"
#include "../rendering/VulkanTools.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "..\..\third-party\tinyobjloader\tiny_obj_loader.h"

//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//
//static const int defaultAssimpFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices) : vertices(vertices), indices(indices)
{
}

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::importFromFile(const std::string filename) {
	vertexCount = 0;
	indexCount = 0;

	if (fileExists(filename)) { //file exists and can be loaded

		glm::vec3 scale(1.0f);
		glm::vec2 uvscale(1.0f);
		glm::vec3 center(0.0f);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		std::string cerr;
		bool cret = tinyobj::LoadObj(&attrib, &shapes, &materials, &cerr, filename.c_str());

		if (!cerr.empty()) { // `err` may contain warning message.
			std::cerr << cerr << std::endl;
		}

		if (!cret) {
			std::cerr << "Failed to load model" << std::endl;
			exit(1);
		}


		int i = 0, vertexIndex = 0;
		for (const auto& shape : shapes) {

			vertices.reserve(shape.mesh.indices.size());
			indices.reserve(shape.mesh.indices.size() * 3);
			std::unordered_map<Vertex, int> uniqueVertices = {};
					
			for (const auto& index : shape.mesh.indices) {
			
				Vertex newVertex = {
					glm::vec3(	attrib.vertices[3 * index.vertex_index + 0],
								attrib.vertices[3 * index.vertex_index + 1],
								attrib.vertices[3 * index.vertex_index + 2]),

					glm::vec3(	attrib.normals[3 * index.normal_index + 0],
								attrib.normals[3 * index.normal_index + 1],
								attrib.normals[3 * index.normal_index + 2]),

					glm::vec2(	attrib.texcoords[2 * index.texcoord_index + 0],
								1.0f - attrib.texcoords[2 * index.texcoord_index + 1]),

					glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
				};
				

				if (uniqueVertices.count(newVertex) == 0) {
					uniqueVertices[newVertex] = (int)vertices.size();
					vertices.push_back(newVertex);
				}

				indices.push_back(uniqueVertices[newVertex]);
			}

			vertexCount = (int) vertices.size();
			indexCount = (int) indices.size();
		}
	}
	else
	{
		printf("Error parsing '%s': '\n", filename.c_str());
		
	}
}



//void Mesh::importFromFile(const std::string filename) {
//	std::vector<Vertex> verts;
//	std::vector<uint16_t> indices;
//
//	Assimp::Importer Importer;
//	const aiScene* pScene;
//
//	pScene = Importer.ReadFile(filename.c_str(), defaultAssimpFlags);
//
//	if (pScene)
//	{
//		parts.clear();
//		parts.resize(pScene->mNumMeshes);
//
//		glm::vec3 scale(1.0f);
//		glm::vec2 uvscale(1.0f);
//		glm::vec3 center(0.0f);
//
//		std::vector<float> vertexBuffer;
//		std::vector<uint32_t> indexBuffer;
//
//		vertexCount = 0;
//		indexCount = 0;
//
//		// Load meshes
//		for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
//		{
//			const aiMesh* paiMesh = pScene->mMeshes[i];
//
//			parts[i] = {};
//			parts[i].vertexBase = vertexCount;
//			parts[i].indexBase = indexCount;
//
//			vertexCount += pScene->mMeshes[i]->mNumVertices;
//
//			aiColor3D pColor(0.f, 0.f, 0.f);
//			pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
//
//			const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
//
//			for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
//			{
//				const aiVector3D* pPos = &(paiMesh->mVertices[j]);
//				const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
//				const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
//				const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
//				const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;
//
//				vertexBuffer.push_back(pPos->x * scale.x + center.x);
//				vertexBuffer.push_back(-pPos->y * scale.y + center.y);
//				vertexBuffer.push_back(pPos->z * scale.z + center.z);
//
//				vertexBuffer.push_back(pNormal->x);
//				vertexBuffer.push_back(-pNormal->y);
//				vertexBuffer.push_back(pNormal->z);
//
//				vertexBuffer.push_back(pTexCoord->x * uvscale.s);
//				vertexBuffer.push_back(pTexCoord->y * uvscale.t);
//
//				vertexBuffer.push_back(pColor.r);
//				vertexBuffer.push_back(pColor.g);
//				vertexBuffer.push_back(pColor.b);
//
//				/*for (auto& component : layout.components)
//				{
//					switch (component) {
//					case VERTEX_COMPONENT_POSITION:
//						vertexBuffer.push_back(pPos->x * scale.x + center.x);
//						vertexBuffer.push_back(-pPos->y * scale.y + center.y);
//						vertexBuffer.push_back(pPos->z * scale.z + center.z);
//						break;
//					case VERTEX_COMPONENT_NORMAL:
//						vertexBuffer.push_back(pNormal->x);
//						vertexBuffer.push_back(-pNormal->y);
//						vertexBuffer.push_back(pNormal->z);
//						break;
//					case VERTEX_COMPONENT_UV:
//						vertexBuffer.push_back(pTexCoord->x * uvscale.s);
//						vertexBuffer.push_back(pTexCoord->y * uvscale.t);
//						break;
//					case VERTEX_COMPONENT_COLOR:
//						vertexBuffer.push_back(pColor.r);
//						vertexBuffer.push_back(pColor.g);
//						vertexBuffer.push_back(pColor.b);
//						break;
//					case VERTEX_COMPONENT_TANGENT:
//						vertexBuffer.push_back(pTangent->x);
//						vertexBuffer.push_back(pTangent->y);
//						vertexBuffer.push_back(pTangent->z);
//						break;
//					case VERTEX_COMPONENT_BITANGENT:
//						vertexBuffer.push_back(pBiTangent->x);
//						vertexBuffer.push_back(pBiTangent->y);
//						vertexBuffer.push_back(pBiTangent->z);
//						break;
//						// Dummy components for padding
//					case VERTEX_COMPONENT_DUMMY_FLOAT:
//						vertexBuffer.push_back(0.0f);
//						break;
//					case VERTEX_COMPONENT_DUMMY_VEC4:
//						vertexBuffer.push_back(0.0f);
//						vertexBuffer.push_back(0.0f);
//						vertexBuffer.push_back(0.0f);
//						vertexBuffer.push_back(0.0f);
//						break;
//					};
//				}*/
//
//				dimensions.max.x = fmax(pPos->x, dimensions.max.x);
//				dimensions.max.y = fmax(pPos->y, dimensions.max.y);
//				dimensions.max.z = fmax(pPos->z, dimensions.max.z);
//
//				dimensions.min.x = fmin(pPos->x, dimensions.min.x);
//				dimensions.min.y = fmin(pPos->y, dimensions.min.y);
//				dimensions.min.z = fmin(pPos->z, dimensions.min.z);
//			}
//
//			dimensions.size = dimensions.max - dimensions.min;
//
//			parts[i].vertexCount = paiMesh->mNumVertices;
//
//			uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
//			for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
//			{
//				const aiFace& Face = paiMesh->mFaces[j];
//				if (Face.mNumIndices != 3)
//					continue;
//				indexBuffer.push_back(indexBase + Face.mIndices[0]);
//				indexBuffer.push_back(indexBase + Face.mIndices[1]);
//				indexBuffer.push_back(indexBase + Face.mIndices[2]);
//				parts[i].indexCount += 3;
//				indexCount += 3;
//			}
//		}
//	}
//}

