#include "Texture.h"

Texture::Texture() {

};

Texture::~Texture() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		throw std::runtime_error("failed to free texture image!");
		std::cerr << "Failed to free pixels, is there a null pointer?" << std::endl;
	}
};

bool Texture::loadTexture(std::string filename, int imgType) {
	std::ifstream filestream(filename.c_str());
	if (filestream.fail()) {
		std::cout << "Could not load texture from " << filename << "File not found" << std::endl;
		return false;
	}
	int texWidth, texHeight, texChannels;
	this->pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, imgType);

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
		return false;
	}

	this->texImageSize = texWidth * texHeight * 4;
	this->width = static_cast<uint32_t>(texWidth);
	this->height = static_cast<uint32_t>(texHeight);
	return true;
}

void Texture::loadFromFileRGBA(std::string filename) {
	loadTexture(filename, STBI_rgb_alpha);
};

void Texture::loadFromFileGreyOnly(std::string filename) {
	loadTexture(filename, STBI_grey);
};

void Texture::loadFromGreyscalePixelData(int width, int height, float* in_pixels) {
	if (in_pixels == nullptr) {
		std::cout << "Noise Utils Image Null, Cannot load null image" << std::endl;
		return;
	}

	int imgWidth = width;
	int imgHeight = height;

	this->width = static_cast<uint32_t>(imgWidth);
	this->height = static_cast<uint32_t>(imgHeight);

	this->texImageSize = imgWidth * imgHeight * 4;

	pixels = (stbi_uc*)malloc(texImageSize);
	if (pixels != nullptr) {
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				pixels[(i * imgHeight + j) * 4 + 0] = (stbi_uc)(in_pixels[i * imgHeight + j] * 128 + 128);
				pixels[(i * imgHeight + j) * 4 + 1] = (stbi_uc)(in_pixels[i * imgHeight + j] * 128 + 128);
				pixels[(i * imgHeight + j) * 4 + 2] = (stbi_uc)(in_pixels[i * imgHeight + j] * 128 + 128);
				pixels[(i * imgHeight + j) * 4 + 3] = 1;
			}
		}
	}
}

void Texture::loadFromRGBAPixelData(int width, int height, RGBA_pixel* in_pixels) {
	this->width = static_cast<uint32_t>(width);
	this->height = static_cast<uint32_t>(height);
	this->texImageSize = width * height * 4;

	this->pixels = (stbi_uc*)in_pixels;
}

TextureArray::TextureArray() {

}

TextureArray::~TextureArray() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		throw std::runtime_error("failed to free texture image!");
		std::cerr << "Failed to free pixels, is there a null pointer?" << std::endl;
	}
}

void TextureArray::loadFromFile(std::string path, std::vector<std::string> filenames){
	std::vector<std::shared_ptr<Texture>> textures;
	textures.reserve(filenames.size());

	for each (std::string name in filenames) {
		std::shared_ptr<Texture> tex = std::make_shared<Texture>();
		tex->loadFromFileRGBA(path + name);
		textures.push_back(tex);
	}

	if (textures.size() == 0) {
		std::cerr << "No images to load. Is this intended?" << std::endl;
		return;
	}

	bool sameSize = true;
	int width = textures.at(0)->width;
	int height = textures.at(0)->height;
	for (auto tex : textures) {
		if (tex->width != width || tex->height != height)
			sameSize = false;
	}
	if (sameSize) {
		this->width = static_cast<uint32_t>(width);
		this->height = static_cast<uint32_t>(height);

		this->texImageSize = textures.size() * textures.at(0)->texImageSize;
		this->texImageSizePerTex = textures.at(0)->texImageSize;
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
	else {
		throw std::runtime_error("Texture array not all same size!");
	}
}

CubeMap::~CubeMap() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		throw std::runtime_error("failed to free texture image!");
	}
}

void CubeMap::loadFromFile(std::string filename, std::string fileExt) {
	Texture Front, Back, Left, Right, Top, Bottom;

	Front.loadFromFileRGBA(filename + "Front" + fileExt);
	Back.loadFromFileRGBA(filename + "Back" + fileExt);
	Right.loadFromFileRGBA(filename + "Right" + fileExt);
	Left.loadFromFileRGBA(filename + "Left" + fileExt);
	Top.loadFromFileRGBA(filename + "Top" + fileExt);
	Bottom.loadFromFileRGBA(filename + "Bottom" + fileExt);
	
	this->width = static_cast<uint32_t>(Front.width);
	this->height = static_cast<uint32_t>(Front.height);

	if (   width == Back.width 
		&& width == Left.width 
		&& width == Right.width 
		&& width == Top.width 
		&& width == Bottom.width

		&& height == Back.height 
		&& height == Left.height 
		&& height == Right.height 
		&& height == Top.height 
		&& height == Bottom.height) {

		this->texImageSize = (Front.texImageSize + Back.texImageSize + Top.texImageSize + Bottom.texImageSize + Right.texImageSize + Left.texImageSize);
		this->texImageSizePerTex = Front.texImageSize;

		this->pixels = (stbi_uc*)malloc(texImageSize);
		if (!pixels) {
			throw std::runtime_error("failed to allocate texture image!");
		}

		stbi_uc* offset = this->pixels;

		std::memcpy(offset, Front.pixels, Front.texImageSize);

		offset += Front.texImageSize;
		std::memcpy(offset, Back.pixels, Back.texImageSize);

		offset += Back.texImageSize;
		std::memcpy(offset, Top.pixels, Top.texImageSize);

		offset += Top.texImageSize;
		std::memcpy(offset, Bottom.pixels, Bottom.texImageSize);

		offset += Bottom.texImageSize;
		std::memcpy(offset, Left.pixels, Left.texImageSize);

		offset += Left.texImageSize;
		std::memcpy(offset, Right.pixels, Right.texImageSize);
	}
	else {
		std::cerr << "Skybox dimentions incorrect! " << std::endl;
	}
};
