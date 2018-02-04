#include "Texture.h"

#include "../core/Logger.h"

Texture::Texture(uint32_t width, uint32_t height)
	:width(width), height(height)
{
	pixels.resize(width*height);
	texImageSize = width * height * 4;
};

TextureArray::TextureArray(uint32_t width, uint32_t height, uint32_t layerCount)
	:width(width), height(height), layerCount(layerCount)
{
	pixels.resize(width*height*layerCount);
	texImageSize = width * height * layerCount * 4;
	texImageSizePerTex = width * height * 4;
}

CubeMap::CubeMap(uint32_t width, uint32_t height) :
	width(width), height(height)
{
	pixels.resize(width*height*6);
	texImageSize = width * height * 6 * 4;
	texImageSizePerTex = width * height * 4;
}

