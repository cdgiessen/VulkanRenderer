#pragma once

#include <string>

#include <glm\common.hpp>

#include <stb_image.h>

class Texture {
public:
	uint32_t width, height;
	uint32_t layerCount = 1;
	uint32_t mipLevels;
	VkDeviceSize imageSize;

	stbi_uc* pixels;

	Texture(){};

	~Texture() {
		stbi_image_free(pixels);
	};

	void loadFromFile(std::string filename) {
		if (!fileExists(filename)) {
			std::cout << "Could not load texture from " << filename << "File not found" << std::endl;
		}
		int texWidth, texHeight, texChannels;
		this->pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		this->imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		this->width = static_cast<uint32_t>(texWidth);
		this->height = static_cast<uint32_t>(texHeight);
		this->mipLevels = static_cast<uint32_t>(1);
	};


};