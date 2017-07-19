#pragma once

#include <vulkan\vulkan.h>

#include <string>
#include <iostream>
#include <fstream>

#include <glm\common.hpp>

#include <stb_image.h>

class Texture {
public:
	uint32_t width, height;
	uint32_t layerCount = 1;
	uint32_t mipLevels;
	VkDeviceSize texImageSize;

	stbi_uc* pixels;

	Texture();
	~Texture();

	void loadFromFile(std::string filename);


};

class CubeMap {
public:
	Texture imageFront;
	Texture imageBack;
	Texture imageLeft;
	Texture imageRight;
	Texture imageTop;
	Texture imageBottom;

	uint32_t width, height;
	uint32_t layerCount = 1;
	uint32_t mipLevels;
	VkDeviceSize texImageSize;

	~CubeMap();

	void loadFromFile(std::string filename, std::string fileExt);

	stbi_uc* pixels;
};