#pragma once

#include <vulkan\vulkan.h>

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <glm\common.hpp>

#include <stb_image.h>
#include "noiseutils.h"

class Texture {
public:
	uint32_t width, height;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize;

	stbi_uc* pixels;

	Texture();
	~Texture();

	void loadFromFileRGBA(std::string filename);
	void loadFromFileGreyOnly(std::string filename);
	void loadFromNoiseUtilImage(utils::Image* image);
	void loadFromGreyscalePixelData(int width, int height, float* pixels);

};

class TextureArray {
public:
	uint32_t width, height;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize;

	std::vector<Texture *> textures;
	stbi_uc* pixels;

	TextureArray();
	~TextureArray();

	void loadFromFile(std::string path, std::vector<std::string> filenames);


};

class CubeMap {
public:
	struct images{
		Texture Front;
		Texture Back;
		Texture Left;
		Texture Right;
		Texture Top;
		Texture Bottom;
	} cubeImages;

	uint32_t width, height;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize;

	~CubeMap();

	void loadFromFile(std::string filename, std::string fileExt);

	stbi_uc* pixels;
};