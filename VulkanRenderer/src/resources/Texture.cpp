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


CubeMap::~CubeMap() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		throw std::runtime_error("failed to free texture image!");
	}
}
