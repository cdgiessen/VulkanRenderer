#include "TextureManager.h"

#include <cstring>

#include "../core/Logger.h"

TextureManager::TextureManager()
{
	errorImage = std::make_shared<Texture>(64,64);
	std::vector<RGBA_pixel> pink(64); //{ { RGBA_pixel(255, 0, 255, 255)} };
	std::vector<RGBA_pixel> black(64); //= { { RGBA_pixel(0, 0, 0, 255)} };
	for (int i = 0; i < 64; i++)
	{
		pink[i] = RGBA_pixel(255, 0, 255, 255);
		black[i] = RGBA_pixel(0, 0, 0, 255);
	}

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if ((i + j) % 2 == 0) {
				std::memcpy(&errorImage->pixels[(i * 8 + j) * 8], pink.data(), 64);
			}
		}
	}
}

TextureManager::~TextureManager()
{
}


std::shared_ptr<Texture> TextureManager::loadTextureFromFile(std::string filename, int imgType) {

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels;
	pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, imgType);
	
	if (pixels == nullptr) {
		Log::Error << "Image failed to load! Was the name correct?\n";
		return errorImage;
	}

	std::shared_ptr<Texture> tex = std::make_shared<Texture>(texWidth, texHeight);

	std::memcpy(tex->pixels.data(), pixels, texWidth * texHeight * 4);

	stbi_image_free(pixels);

	textureHandles.push_back(tex);
	return tex;
}

std::shared_ptr<Texture> TextureManager::loadTextureFromFileRGBA(std::string filename) {
	return loadTextureFromFile(filename, STBI_rgb_alpha);
};
std::shared_ptr<Texture> TextureManager::loadTextureFromFileGreyOnly(std::string filename) {
	return loadTextureFromFile(filename, STBI_grey);
};

std::shared_ptr<Texture> TextureManager::loadTextureFromGreyscalePixelData(int width, int height, float* in_pixels) {
	if (width < 0 || height < 0) {
		Log::Error << "Can't have negative dimentions!\n";
		return nullptr;
	}

	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);
	
	if (in_pixels == nullptr) {
		Log::Debug << "Noise Utils Image Null, Cannot load null image!\n";
		return errorImage;
	}

	
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			tex->pixels[i * width + j] = RGBA_pixel(
				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
				1);
		}
	}
	

	textureHandles.push_back(tex);
	
	return tex;
}

std::shared_ptr<Texture> TextureManager::loadTextureFromRGBAPixelData(int width, int height, RGBA_pixel* in_pixels) {
	if (width < 0 || height < 0) {
		Log::Error << "Can't have negative dimentions!\n";
		return errorImage;
	}
	
	if (in_pixels == nullptr) {
		Log::Debug << "Noise Utils Image Null, Cannot load null image!\n";
		return errorImage;
	}

	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);
	
	std::memcpy(tex->pixels.data(), in_pixels, tex->texImageSize);	

	return tex;
}


std::shared_ptr<TextureArray> TextureManager::loadTextureArrayFromFile(std::string path, std::vector<std::string> filenames) {
	std::shared_ptr<TextureArray> texArray;
	std::vector<std::shared_ptr<Texture>> textures;
	textures.reserve(filenames.size());

	for (std::string name : filenames) {
		auto tex = loadTextureFromFileRGBA(path + name);
		if (tex != nullptr) {
			textures.push_back(tex);
		}
	}

	if (textures.size() == 0) {
		Log::Error << "No images to load. Is this intended?\n";
	}

	bool sameSize = true;
	int width = textures.at(0)->width;
	int height = textures.at(0)->height;
	for (auto tex : textures) {
		if (tex->width != width || tex->height != height)
			sameSize = false;
	}
	if (sameSize) {
		texArray = std::make_shared < TextureArray>(width, height, textures.size());

		for (int l = 0; l < textures.size(); l++)
		{
			std::memcpy(texArray->pixels.data() + l * width*height, textures.at(l)->pixels.data(), width*height * 4);
		}
	}
	else {
		throw std::runtime_error("Texture array not all same size!");
	}

	return texArray;
}


std::shared_ptr<CubeMap> TextureManager::loadCubeMapFromFile(std::string filename, std::string fileExt) {
	std::shared_ptr<CubeMap> cubeMap;
	std::vector<std::shared_ptr<Texture>> faces;

	faces.push_back(loadTextureFromFileRGBA(filename + "Front" + fileExt));
	faces.push_back(loadTextureFromFileRGBA(filename + "Back" + fileExt));
	faces.push_back(loadTextureFromFileRGBA(filename + "Top" + fileExt));
	faces.push_back(loadTextureFromFileRGBA(filename + "Bottom" + fileExt));
	faces.push_back(loadTextureFromFileRGBA(filename + "Right" + fileExt));
	faces.push_back(loadTextureFromFileRGBA(filename + "Left" + fileExt));

	bool sameSize = true;
	int width = faces.at(0)->width;
	int height = faces.at(0)->height;
	for (auto tex : faces) {
		if (tex->width != width || tex->height != height)
			sameSize = false;
	}
	if (sameSize) {
		cubeMap = std::make_shared<CubeMap>(width, height);

		for (int l = 0; l < faces.size(); l++)
		{
			std::memcpy(cubeMap->pixels.data() + l * width * height, faces.at(l)->pixels.data(), width * height * 4);
		}

	}
	else {
		throw std::runtime_error("Cubemap faces not all same size!");
	}

	return cubeMap;
};
