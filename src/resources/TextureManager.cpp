#include "TextureManager.h"

#include <cstring>

#include "../core/Logger.h"

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}


std::shared_ptr<Texture> TextureManager::loadTextureFromFile(std::string filename, int imgType) {
	std::shared_ptr<Texture> tex = std::make_shared<Texture>();

	std::ifstream filestream(filename.c_str());
	if (filestream.fail()) {
		Log::Debug << "Could not load texture from " << filename << "File not found! \n";
		return std::shared_ptr<Texture>(nullptr);
	}
	int texWidth, texHeight, texChannels;
	tex->pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, imgType);

	if (!tex->pixels) {
		throw std::runtime_error("failed to load texture image!");
		return std::shared_ptr<Texture>(nullptr);
	}

	tex->texImageSize = texWidth * texHeight * 4;
	tex->width = static_cast<uint32_t>(texWidth);
	tex->height = static_cast<uint32_t>(texHeight);

	textureHandles.push_back(tex);

	return tex;
}

std::shared_ptr<Texture> TextureManager::loadTextureFromFileRGBA(std::string filename) {
	return loadTextureFromFile(filename, STBI_rgb_alpha);
};
std::shared_ptr<Texture> TextureManager::loadTextureFromFileGreyOnly(std::string filename) {
	return loadTextureFromFile(filename, STBI_grey);
};

std::shared_ptr<Texture> TextureManager::loadTextureFromPixelData(int width, int height) {

	if (width < 0 || height < 0) {
		Log::Error << "Can't have negative dimentions!\n";
		return nullptr;
	}
	
	std::shared_ptr<Texture> tex = std::make_shared<Texture>();

	tex->width = static_cast<uint32_t>(width);
	tex->height = static_cast<uint32_t>(height);

	tex->texImageSize = width * height * 4;

	textureHandles.push_back(tex);

	return tex;
}

std::shared_ptr<Texture> TextureManager::loadTextureFromGreyscalePixelData(int width, int height, float* in_pixels) {
	auto tex = loadTextureFromPixelData(width, height);
	
	if (in_pixels == nullptr) {
		Log::Debug << "Noise Utils Image Null, Cannot load null image!\n";
		return std::shared_ptr<Texture>(nullptr);
	}

	tex->pixels = (stbi_uc*)malloc(tex->texImageSize);
	if (tex->pixels != nullptr) {
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				tex->pixels[(i * tex->height + j) * 4 + 0] = (stbi_uc)(in_pixels[i * tex->height + j] * 128 + 128);
				tex->pixels[(i * tex->height + j) * 4 + 1] = (stbi_uc)(in_pixels[i * tex->height + j] * 128 + 128);
				tex->pixels[(i * tex->height + j) * 4 + 2] = (stbi_uc)(in_pixels[i * tex->height + j] * 128 + 128);
				tex->pixels[(i * tex->height + j) * 4 + 3] = 1;
			}
		}
	}
	else {
		throw std::runtime_error("Failed to allocate memory for texture! Did we run out?");
	}

	return tex;
}

std::shared_ptr<Texture> TextureManager::loadTextureFromRGBAPixelData(int width, int height, RGBA_pixel* in_pixels) {
	auto tex = loadTextureFromPixelData(width, height);
		
	tex->pixels = (stbi_uc*)in_pixels;

	return tex;
}


std::shared_ptr<TextureArray> TextureManager::loadTextureArrayFromFile(std::string path, std::vector<std::string> filenames) {
	std::shared_ptr<TextureArray> texArray = std::make_shared < TextureArray>();
	
	std::vector<std::shared_ptr<Texture>> textures;
	textures.reserve(filenames.size());

	for (std::string name : filenames) {
		auto tex = loadTextureFromFileRGBA(path + name);
		textures.push_back(tex);
	}

	if (textures.size() == 0) {
		Log::Error << "No images to load. Is this intended?\n";
		return std::shared_ptr<TextureArray>(nullptr);
	}

	bool sameSize = true;
	int width = textures.at(0)->width;
	int height = textures.at(0)->height;
	for (auto tex : textures) {
		if (tex->width != width || tex->height != height)
			sameSize = false;
	}
	if (sameSize) {
		texArray->width = static_cast<uint32_t>(width);
		texArray->height = static_cast<uint32_t>(height);
		
		texArray->texImageSize = textures.size() * textures.at(0)->texImageSize;
		texArray->texImageSizePerTex = textures.at(0)->texImageSize;
		texArray->layerCount = (uint32_t)textures.size();

		stbi_uc* pix = (stbi_uc*)malloc(texArray->texImageSize);
		stbi_uc* offset = pix;

		for (uint32_t i = 0; i < texArray->layerCount; i++)
		{
			memcpy(offset, textures.at(i)->pixels, textures.at(i)->texImageSize);
			offset += textures.at(i)->texImageSize;
		}

		texArray->pixels = pix;
	}
	else {
		throw std::runtime_error("Texture array not all same size!");
	}

	return texArray;
}


std::shared_ptr<CubeMap> TextureManager::loadCubeMapFromFile(std::string filename, std::string fileExt) {
	std::shared_ptr<CubeMap> cubeMap = std::make_shared<CubeMap>();

	auto Front = loadTextureFromFileRGBA(filename + "Front" + fileExt);
	auto Back = loadTextureFromFileRGBA(filename + "Back" + fileExt);
	auto Right = loadTextureFromFileRGBA(filename + "Right" + fileExt);
	auto Left = loadTextureFromFileRGBA(filename + "Left" + fileExt);
	auto Top = loadTextureFromFileRGBA(filename + "Top" + fileExt);
	auto Bottom = loadTextureFromFileRGBA(filename + "Bottom" + fileExt);

	cubeMap->width = static_cast<uint32_t>(Front->width);
	cubeMap->height = static_cast<uint32_t>(Front->height);

	if (cubeMap->width == Back->width
		&& cubeMap->width == Left->width
		&& cubeMap->width == Right->width
		&& cubeMap->width == Top->width
		&& cubeMap->width == Bottom->width

		&& cubeMap->height == Back->height
		&& cubeMap->height == Left->height
		&& cubeMap->height == Right->height
		&& cubeMap->height == Top->height
		&& cubeMap->height == Bottom->height) {

		cubeMap->texImageSize = (Front->texImageSize + Back->texImageSize + Top->texImageSize + Bottom->texImageSize + Right->texImageSize + Left->texImageSize);
		cubeMap->texImageSizePerTex = Front->texImageSize;

		cubeMap->pixels = (stbi_uc*)malloc(cubeMap->texImageSize);
		if (!cubeMap->pixels) {
			throw std::runtime_error("failed to allocate texture image!");
		}

		stbi_uc* offset = cubeMap->pixels;

		memcpy(offset, Front->pixels, Front->texImageSize);

		offset += Front->texImageSize;
		memcpy(offset, Back->pixels, Back->texImageSize);

		offset += Back->texImageSize;
		memcpy(offset, Top->pixels, Top->texImageSize);

		offset += Top->texImageSize;
		memcpy(offset, Bottom->pixels, Bottom->texImageSize);

		offset += Bottom->texImageSize;
		memcpy(offset, Left->pixels, Left->texImageSize);

		offset += Left->texImageSize;
		memcpy(offset, Right->pixels, Right->texImageSize);
	}
	else {
		Log::Error << "Skybox dimentions incorrect! \n";
	}

	return cubeMap;
};
