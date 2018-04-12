#pragma once

#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

class VulkanTexture;

/* Synchronization */

using Signal = std::shared_ptr<bool>;

/* Common uniform buffers */

struct GlobalData {
	float time;
};

struct CameraData {
	glm::mat4 projView;
	glm::mat4 view;
	glm::vec3 cameraDir;
	glm::vec3 cameraPos;
} ;

/* Lighting */

struct Light_Directional {
	glm::vec3 sunDir;
	float sunIntensity;
	glm::vec3 sunColor;
};

struct Light_Point {
	glm::vec3 position;
	glm::vec3 color;
	float attenuation;
};

struct  Light_Spot {
	glm::vec3 position;
	glm::vec3 color;
	float attenuation;
	float cutoff;
	float outerCutOff;
};







struct GlobalVariableUniformBuffer {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 cameraDir;
	float time;
	glm::vec3 sunDir;
	float sunIntensity;
	glm::vec3 sunColor;
};

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



/* Lighting */

struct PointLight {
	glm::vec4 lightPos = glm::vec4(50.0f, 25.0f, 50.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	PointLight() {};
	PointLight(glm::vec4 pos, glm::vec4 color, glm::vec4 atten) : lightPos(pos), color(color), attenuation(atten) {};
};


struct DirectionalLight {
	glm::vec4 lightDir = glm::vec4(50.0f, -65.0f, 50.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0, 1.0, 1.0f, 1.0f);
	glm::vec4 attenuation = glm::vec4(1.0, 0.007f, 0.0002f, 1.0f);

	DirectionalLight() {};
	DirectionalLight(glm::vec4 dir, glm::vec4 color, glm::vec4 atten) : lightDir(dir), color(color), attenuation(atten) {};
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

/* Materials */

struct PBR_Material {
	glm::vec3  albedo;
	float metallic;
	float roughness;
	float ao;
	glm::vec3 emmisive;

	bool useTexAlbedo;
	bool useTexMetallic;
	bool useTexRoughness;
	bool useTexAmbientOcclusion;
	bool useTexEmmisive;
	bool useTexNormal;

	std::shared_ptr<VulkanTexture> tx_albedo;
	std::shared_ptr<VulkanTexture> tx_metallic;
	std::shared_ptr<VulkanTexture> tx_roughness;
	std::shared_ptr<VulkanTexture> tx_ao;
	std::shared_ptr<VulkanTexture> tx_emissiveTexture;
	std::shared_ptr<VulkanTexture> tx_normal;
};

const glm::mat4 depthReverserMatrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1);