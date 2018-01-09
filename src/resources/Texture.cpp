#include "Texture.h"

#include "../core/Logger.h"

Texture::Texture() {

};

Texture::~Texture() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		Log::Error << "Failed to free pixels, is there a null pointer?" << "\n";
		throw std::runtime_error("failed to free texture image!");
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
		Log::Error << "Failed to free pixels, is there a null pointer?" << "\n";
		throw std::runtime_error("failed to free texture image!");
	}
}


CubeMap::~CubeMap() {
	if (pixels) {
		free(pixels);
		pixels = 0;
	}
	else {
		Log::Error << "Failed to free pixels, is there a null pointer?" << "\n";
		throw std::runtime_error("failed to free texture image!");
	}
}
