#include "Mesh.h"

#include "../core/CoreTools.h"
#include "../core/Logger.h"

#include "../rendering/Initializers.h"

Mesh::Mesh(Vertices_PosNorm vertices, std::vector<uint16_t> indices)
	: vertices(vertices), indices(indices), vertexElementCount(6)
{
	//this->vertices = vertices;
}

Mesh::Mesh(Vertices_PosNormTex vertices, std::vector<uint16_t> indices)
	: vertices(vertices), indices(indices), vertexElementCount(8)
{
	//this->vertices = vertices;

}

Mesh::Mesh(Vertices_PosNormTexColor vertices, std::vector<uint16_t> indices)
	: vertices(vertices), indices(indices), vertexElementCount(12)
{
	//this->vertices = vertices;

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

	// if (fileExists(filename)) { //file exists and can be loaded

	// 	glm::vec3 scale(1.0f);
	// 	glm::vec2 uvscale(1.0f);
	// 	glm::vec3 center(0.0f);

	// 	tinyobj::attrib_t attrib;
	// 	std::vector<tinyobj::shape_t> shapes;
	// 	std::vector<tinyobj::material_t> materials;

	// 	std::string cerr;
	// 	bool cret = tinyobj::LoadObj(&attrib, &shapes, &materials, &cerr, filename.c_str());

	// 	if (!cerr.empty()) { // `err` may contain warning message.
	// 		Log::Error << cerr << "\n";
	// 	}

	// 	if (!cret) {
	// 		Log::Error << "Failed to load model" << "\n";
	// 		exit(1);
	// 	}


	// 	int i = 0, vertexIndex = 0;
	// 	for (const auto& shape : shapes) {

	// 		vertices.reserve(shape.mesh.indices.size());
	// 		indices.reserve(shape.mesh.indices.size() * 3);
	// 		std::unordered_map<Vertex, int> uniqueVertices = {};

	// 		for (const auto& index : shape.mesh.indices) {

	// 			Vertex newVertex = {
	// 				glm::vec3(	attrib.vertices[3 * index.vertex_index + 0],
	// 							attrib.vertices[3 * index.vertex_index + 1],
	// 							attrib.vertices[3 * index.vertex_index + 2]),

	// 				glm::vec3(	attrib.normals[3 * index.normal_index + 0],
	// 							attrib.normals[3 * index.normal_index + 1],
	// 							attrib.normals[3 * index.normal_index + 2]),

	// 				glm::vec2(	attrib.texcoords[2 * index.texcoord_index + 0],
	// 							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]),

	// 				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	// 			};


	// 			if (uniqueVertices.count(newVertex) == 0) {
	// 				uniqueVertices[newVertex] = (int)vertices.size();
	// 				vertices.push_back(newVertex);
	// 			}

	// 			indices.push_back(uniqueVertices[newVertex]);
	// 		}

	// 		vertexCount = (int) vertices.size();
	// 		indexCount = (int) indices.size();
	// 	}
	// }
	// else
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


MeshManager::MeshManager()
{
}


MeshManager::~MeshManager()
{
}


std::shared_ptr<Mesh> createSinglePlane() {
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({
		//vertices

		{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 1.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }

		//indices
		}), std::vector<uint16_t>({
			0, 1, 2, 2, 3, 0
			}));
};

std::shared_ptr<Mesh> createDoublePlane() {
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({
		//vertices

		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },

		{ { -0.5f, -0.5f, -1.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -1.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.5f, 0.5f, -1.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -1.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }

		//indices
		}), std::vector<uint16_t>({
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
			}));
};


std::shared_ptr<Mesh> createFlatPlane(int dim, glm::vec3 size) {
	Vertices_PosNormTex verts;
	std::vector<uint16_t> indices;

	verts.resize((dim + 1) * (dim + 1));
	indices.resize((dim) * (dim) * 6);

	for (int i = 0; i <= dim; i++)
	{
		for (int j = 0; j <= dim; j++)
		{
			verts[(i)*(dim + 1) + j] = Vertex_PosNormTex(
				glm::vec3((double)i *(size.x) / (float)dim, 0,
				(double)j *(size.z) / (float)dim),
				glm::vec3(0, 1, 0), glm::vec2(i, j));
		}
	}

	int counter = 0;
	for (int i = 0; i < dim; i++)
	{
		for (int j = 0; j < dim; j++)
		{
			indices[counter++] = i * (dim + 1) + j;
			indices[counter++] = i * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j;
			indices[counter++] = i * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j + 1;
			indices[counter++] = (i + 1) * (dim + 1) + j;
		}
	}

	return std::make_shared<Mesh>(verts, indices);
}

std::shared_ptr<Mesh> createCube() {
	return std::make_shared<Mesh>(Vertices_PosNormTexColor({

		//Left face
		{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.333f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.667f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f, -1.0f },{ 0.667f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },

		//Right face			
		{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.667f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.667f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.333f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f },{ 0.0f, 0.0f, 1.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },

		//Back face			
		{ { -0.5f, -0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.333f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.333f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f },{ -1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },

		//Front face																  
		{ { 0.5f, 0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.334f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.334f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.334f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },

		//Bottom face		
		{ { 0.5f, -0.5f, 0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.667f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.5f },{ 0.0f, -1.0f, 0.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, -0.5f, -0.5f },{ 0.0f, -1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f,1.0f,1.0f, 1.0f } },

		//Top face			
		{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f },{ 0.667f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f,1.0f,1.0f, 1.0f } },
		{ { -0.5f, 0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 0.667f, 0.5f },{ 1.0f,1.0f,1.0f, 1.0f } }


		//indices
		}), std::vector<uint16_t>({
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
			}));

}


void AddPlane(Vertices_PosNormTex& verts, std::vector<uint16_t>& indices,
	int dim, int faceNum,
	glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight
) {

	glm::vec3 p1 = topLeft;
	glm::vec3 p2 = topRight;
	glm::vec3 p3 = bottomLeft;

	glm::vec3 t1 = p2 - p1;
	glm::vec3 t2 = p3 - p1;

	glm::vec3 normal = glm::cross(t1, t2);

	for (float i = 0; i <= dim; i++) {
		for (float j = 0; j <= dim; j++) {

			verts.push_back(
				Vertex_PosNormTex(glm::mix(glm::mix(topLeft, topRight, j / dim), glm::mix(bottomLeft, bottomRight, j / dim), i / dim),
					normal, glm::vec2(i, j)));
		}
	}

	for (int i = 0; i < dim; i++)
	{
		for (int j = 0; j < dim; j++)
		{
			indices.push_back((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j);
			indices.push_back((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j + 1);
			indices.push_back((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j);
			indices.push_back((dim + 1) * (dim + 1) * faceNum + i * (dim + 1) + j + 1);
			indices.push_back((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j + 1);
			indices.push_back((dim + 1) * (dim + 1) * faceNum + (i + 1) * (dim + 1) + j);
		}
	}
}

std::shared_ptr<Mesh> createSphere(int dim) {

	Vertices_PosNormTex verts;
	std::vector<uint16_t> indices;

	verts.reserve((dim + 1) * (dim + 1) * 6);
	indices.reserve((dim) * (dim) * 6 * 6);

	AddPlane(verts, indices, dim, 0, glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, -0.5));
	AddPlane(verts, indices, dim, 1, glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5));
	AddPlane(verts, indices, dim, 2, glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5));
	AddPlane(verts, indices, dim, 3, glm::vec3(-0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, -0.5));
	AddPlane(verts, indices, dim, 4, glm::vec3(-0.5, 0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0.5, -0.5, 0.5));
	AddPlane(verts, indices, dim, 5, glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, -0.5, -0.5));

	for (auto& vert : verts) {
		vert.pos = glm::normalize(vert.pos);
		vert.normal = vert.pos;
	}

	return std::make_shared<Mesh>(verts, indices);
};