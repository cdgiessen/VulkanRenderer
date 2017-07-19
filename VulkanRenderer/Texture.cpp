#include "Texture.h"

#include "VulkanTools.h"

Texture::Texture() {};

Texture::~Texture() {
	stbi_image_free(pixels);
};

void Texture::loadFromFile(std::string filename) {
	if (!fileExists(filename)) {
		std::cout << "Could not load texture from " << filename << "File not found" << std::endl;
	}
	int texWidth, texHeight, texChannels;
	this->pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	this->texImageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	this->width = static_cast<uint32_t>(texWidth);
	this->height = static_cast<uint32_t>(texHeight);
	this->mipLevels = static_cast<uint32_t>(1);
};

CubeMap::~CubeMap() {

}

void CubeMap::loadFromFile(std::string filename, std::string fileExt) {
	imageFront.loadFromFile(filename + "Front" + fileExt);
	imageBack.loadFromFile(filename + "Back" + fileExt);
	imageLeft.loadFromFile(filename + "Left" + fileExt);
	imageRight.loadFromFile(filename + "Right" + fileExt);
	imageTop.loadFromFile(filename + "Top" + fileExt);
	imageBottom.loadFromFile(filename + "Bottom" + fileExt);

	this->width = static_cast<uint32_t>(imageFront.width);
	this->height = static_cast<uint32_t>(imageFront.height);
	this->mipLevels = static_cast<uint32_t>(imageFront.mipLevels);

	this->texImageSize = (imageFront.texImageSize + imageBack.texImageSize + imageTop.texImageSize + imageBottom.texImageSize + imageRight.texImageSize + imageLeft.texImageSize);

	stbi_uc* pix = (stbi_uc*)malloc(texImageSize);
	int offset = 0;
	memcpy(pix + offset, imageFront.pixels, imageFront.texImageSize);
	offset += imageFront.texImageSize;
	memcpy(pix + offset , imageBack.pixels, imageBack.texImageSize);
	offset += imageBack.texImageSize;
	memcpy(pix + offset, imageTop.pixels, imageTop.texImageSize);
	offset += imageTop.texImageSize;
	memcpy(pix + offset, imageBottom.pixels, imageBottom.texImageSize);
	offset += imageBottom.texImageSize;
	memcpy(pix + offset, imageLeft.pixels, imageLeft.texImageSize);
	offset += imageLeft.texImageSize;
	memcpy(pix + offset, imageRight.pixels, imageRight.texImageSize);

	this->pixels = pix;
};
