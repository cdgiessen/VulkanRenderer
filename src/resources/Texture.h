#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include <vulkan\vulkan.h>

#include <glm\common.hpp>

#include "..\..\third-party\stb_image\stb_image.h"

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

};

class CubeMap {
public:

	uint32_t width = 0, height = 0;
	uint32_t layerCount = 1;
	VkDeviceSize texImageSize = 0;
	VkDeviceSize texImageSizePerTex = 0;

	~CubeMap();

	stbi_uc* pixels = 0;
};