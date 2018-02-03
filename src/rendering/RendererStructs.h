#pragma once

#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "../resources/Texture.h"

#include "../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

/* Common uniform buffers */

//Global info buffer
struct GlobalVariableUniformBuffer {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cameraDir;
	float time;
};

//model specific data (position, normal matrix)
struct ModelBufferObject {
	glm::mat4 model;
	glm::mat4 normal;

	glm::mat4 paddingOne;
	glm::mat4 paddingTwo;
};

struct ModelPushConstant {
	glm::mat4 model;
	glm::mat4 normal;
};

struct ModelInfo {
	std::string name;

	ModelBufferObject mbo;
};

//Lighting struct
struct PointLight {
	glm::vec4 lightPos = glm::vec4(50.0f, 25.0f, 50.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	PointLight() {};
	PointLight(glm::vec4 pos, glm::vec4 color, glm::vec4 atten) : lightPos(pos), color(color), attenuation(atten) {};
};

struct PointLightInfo {
	std::string name;
	PointLight pl;
};

struct DirectionalLight {
	glm::vec4 lightDir = glm::vec4(50.0f, -65.0f, 50.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	DirectionalLight() {};
	DirectionalLight(glm::vec4 dir, glm::vec4 color, glm::vec4 atten) : lightDir(dir), color(color), attenuation(atten) {};
};

struct DirectionalLightInfo {
	std::string name;
	DirectionalLight dl;
};

struct SpotLight {
	glm::vec4 lightPos = glm::vec4(50.0f, 25.0f, 50.0f, 1.0f);
	glm::vec4 lightDir = glm::vec4(50.0f, -65.0f, 50.0f, 1.0f);

	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	float cutOff = 0.97f;
	float outerCutOff = 0.95f;

	SpotLight() {};
	SpotLight(glm::vec4 pos, glm::vec4 dir, glm::vec4 color, glm::vec4 atten, float cutOff, float outerCutOff) 
		: lightDir(dir), color(color), attenuation(atten), cutOff(cutOff), outerCutOff(outerCutOff) {};
};

struct SpotLightInfo {
	std::string name;
	SpotLight dl;
};

struct PBRMaterial {
	glm::vec4 baseColor;
	glm::vec4 emissiveFactor;

	float metallicFactor;
	float roughnessFactor;

	std::shared_ptr<Texture> baseColorTexture;
	std::shared_ptr<Texture> metallicRoughnessTexture;

	std::shared_ptr<Texture> normalTexture;

	std::shared_ptr<Texture> ambientOcclusionTexture;

	std::shared_ptr<Texture> emissiveTexture;

};

struct PBRMaterialInfo {
	std::string name;

	PBRMaterial mat;
};