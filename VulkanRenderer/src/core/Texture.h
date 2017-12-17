#pragma once

#include <vulkan\vulkan.h>

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include <glm\common.hpp>

#include "../third-party/stb_image/stb_image.h"

struct RGBA_pixel {
	stbi_uc red = 0;
	stbi_uc green = 0;
	stbi_uc blue = 0;
	stbi_uc alpha = 1;

	RGBA_pixel() {};
	RGBA_pixel(stbi_uc red, stbi_uc green, stbi_uc blue, stbi_uc alpha) : red(red), green(green), blue(blue), alpha(alpha) {  };

};


class Texture {
public:
	uint32_t width = 0, height = 0;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize = 0;

	stbi_uc* pixels = 0;

	Texture();
	~Texture();

	void loadFromFileRGBA(std::string filename);
	void loadFromFileGreyOnly(std::string filename);
	//void loadFromNoiseUtilImage(utils::Image* image);
	void loadFromGreyscalePixelData(int width, int height, float* pixels);
	void loadFromRGBAPixelData(int width, int height, RGBA_pixel* pixels);

private:
	bool loadTexture(std::string, int imgType);
};

class TextureArray {
public:
	uint32_t width = 0, height = 0;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize = 0;
	VkDeviceSize texImageSizePerTex = 0;

	stbi_uc* pixels = 0;

	TextureArray();
	~TextureArray();

	void loadFromFile(std::string path, std::vector<std::string> filenames);
};

class CubeMap {
public:

	uint32_t width = 0, height = 0;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize = 0;
	VkDeviceSize texImageSizePerTex = 0;

	~CubeMap();

	void loadFromFile(std::string filename, std::string fileExt);

	stbi_uc* pixels = 0;
};