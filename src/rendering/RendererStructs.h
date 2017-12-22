#pragma once

#include <vector>
#include <memory>

#include <glm\common.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "..\resources\Texture.h"

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
};

//Lighting struct
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

struct PBRMaterial {
	std::string name;

	glm::vec4 baseColor;
	std::shared_ptr<Texture> baseColorTexture;

	float metallicFactor;
	float roughnessFactor;
	std::shared_ptr<Texture> metallicRoughnessTexture;

	std::shared_ptr<Texture> normalTexture;

	std::shared_ptr<Texture> ambientOcclusionTexture;

	glm::vec4 emissiveFactor;
	std::shared_ptr<Texture> emissiveTexture;

};