#pragma once

#include <string>

#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "../../third-party/stb_image/stb_image.h"
#include "../../third-party/stb_image/stb_image_write.h"


#include "Texture.h"




class TextureManager {
public:
	TextureManager();
	~TextureManager();


	std::shared_ptr<Texture> loadTextureFromFileRGBA(std::string filename);
	std::shared_ptr<Texture> loadTextureFromFileGreyOnly(std::string filename);
	//void loadFromNoiseUtilImage(utils::Image* image);
	std::shared_ptr<Texture> loadTextureFromGreyscalePixelData(int width, int height, float* pixels);
	std::shared_ptr<Texture> loadTextureFromRGBAPixelData(int width, int height, RGBA_pixel* pixels);

	std::shared_ptr<TextureArray> loadTextureArrayFromFile(std::string path, std::vector<std::string> filenames);

	std::shared_ptr<CubeMap> loadCubeMapFromFile(std::string filename, std::string fileExt);


private:
	std::vector<std::shared_ptr<Texture>> textureHandles;

	std::shared_ptr<Texture> loadTextureFromFile(std::string filename, int imgType);
	//std::shared_ptr<Texture> loadTextureFromPixelData(int width, int height); //doesn't copy pixels, but it sets up everthing else

	std::shared_ptr<Texture> errorImage;
	std::vector<RGBA_pixel> errorImageData;
};