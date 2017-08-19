#include "Texture.h"

#include "VulkanTools.h"

Texture::Texture() {};

Texture::~Texture() {
	stbi_image_free(pixels);
};

void Texture::loadFromFile(std::string filename) {
	if (!fileExists(filename)) {
		std::cout << "Could not load texture from " << filename << "File not found" << std::endl;
		return;
	}
	int texWidth, texHeight, texChannels;
	this->pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	this->texImageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	this->width = static_cast<uint32_t>(texWidth);
	this->height = static_cast<uint32_t>(texHeight);
};

void Texture::loadFromNoiseUtilImage(utils::Image* image) {
	if (image->GetSlabPtr() == nullptr) {
		std::cout << "Noise Utils Image Null, Cannot load null image" << std::endl;
		return;
	}

	int imgWidth = image->GetWidth();
	int imgHeight = image->GetHeight();

	this->width = static_cast<uint32_t>(imgWidth);
	this->height = static_cast<uint32_t>(imgHeight);

	this->texImageSize = imgWidth * imgHeight * 4;

	pixels = (stbi_uc*) malloc(texImageSize);

	for (int i = 0; i < imgHeight; i++)
	{
		for (int j = 0; j < imgWidth; j++)
		{
			pixels[(i * imgHeight + j)*4 + 0] = image->GetValue(j, i).red;
			pixels[(i * imgHeight + j)*4 + 1] = image->GetValue(j, i).green;
			pixels[(i * imgHeight + j)*4 + 2] = image->GetValue(j, i).blue;
			pixels[(i * imgHeight + j)*4 + 3] = image->GetValue(j, i).alpha;
		}
	}
	this->width = static_cast<uint32_t>(imgWidth);
	this->height = static_cast<uint32_t>(imgHeight);

	this->texImageSize = imgWidth * imgHeight * 4;
}

TextureArray::TextureArray() {

}

TextureArray::~TextureArray() {
	for each (Texture* tex in textures) {
		tex->~Texture();
	}
}

void TextureArray::loadFromFile(std::string path, std::vector<std::string> filenames){
	for each (std::string name in filenames) {
		Texture* tex = new Texture();
		tex->loadFromFile(path + name);
		textures.push_back(tex);
	}

	this->width = static_cast<uint32_t>(textures.at(0)->width);
	this->height = static_cast<uint32_t>(textures.at(0)->height);

	this->texImageSize = textures.size() * textures.at(0)->texImageSize;
	this->layerCount = textures.size();

	stbi_uc* pix = (stbi_uc*)malloc(texImageSize);
	stbi_uc* offset = pix;

	for (int i = 0; i < layerCount; i++)
	{
		std::memcpy(offset, textures.at(i)->pixels, textures.at(i)->texImageSize);
		offset += textures.at(i)->texImageSize;
	}

	this->pixels = pix;
}

CubeMap::~CubeMap() {
	cubeImages.Front.~Texture();
	cubeImages.Back.~Texture();
	cubeImages.Left.~Texture();
	cubeImages.Right.~Texture();
	cubeImages.Top.~Texture();
	cubeImages.Bottom.~Texture();

	free(pixels);
}

void CubeMap::loadFromFile(std::string filename, std::string fileExt) {
	cubeImages.Front.loadFromFile(filename + "Front" + fileExt);
	cubeImages.Back.loadFromFile(filename + "Back" + fileExt);
	cubeImages.Left.loadFromFile(filename + "Left" + fileExt);
	cubeImages.Right.loadFromFile(filename + "Right" + fileExt);
	cubeImages.Top.loadFromFile(filename + "Top" + fileExt);
	cubeImages.Bottom.loadFromFile(filename + "Bottom" + fileExt);

	this->width = static_cast<uint32_t>(cubeImages.Front.width);
	this->height = static_cast<uint32_t>(cubeImages.Front.height);

	this->texImageSize = (cubeImages.Front.texImageSize + cubeImages.Back.texImageSize + cubeImages.Top.texImageSize + cubeImages.Bottom.texImageSize + cubeImages.Right.texImageSize + cubeImages.Left.texImageSize);

	stbi_uc* pix = (stbi_uc*)malloc(texImageSize);
	stbi_uc* offset = pix;
	std::memcpy(offset, cubeImages.Front.pixels, cubeImages.Front.texImageSize);

	offset += cubeImages.Front.texImageSize;
	std::memcpy(offset, cubeImages.Back.pixels, cubeImages.Back.texImageSize);

	offset += cubeImages.Back.texImageSize;
	std::memcpy(offset, cubeImages.Top.pixels, cubeImages.Top.texImageSize);

	offset += cubeImages.Top.texImageSize;
	std::memcpy(offset, cubeImages.Bottom.pixels, cubeImages.Bottom.texImageSize);

	offset += cubeImages.Bottom.texImageSize;
	std::memcpy(offset, cubeImages.Left.pixels, cubeImages.Left.texImageSize);

	offset += cubeImages.Left.texImageSize;
	std::memcpy(offset, cubeImages.Right.pixels, cubeImages.Right.texImageSize);

	this->pixels = pix;
};
