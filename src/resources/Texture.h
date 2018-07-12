#pragma once

#include <string>

#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstddef>
#include <unordered_map>

#include <glm/common.hpp>

#include <vulkan/vulkan.h>

#include <json.hpp>

namespace Resource::Texture {

	using TexID = int;

	struct Pixel_R {
		std::byte red;

		Pixel_R() {};
		Pixel_R(std::byte red) :
			red(red) {  };
	};

	struct Pixel_RG {
		std::byte red, green;

		Pixel_RG() {};
		Pixel_RG(std::byte red, std::byte green) :
			red(red), green(green) {  };
	};

	struct Pixel_RGB {
		std::byte red, green, blue;

		Pixel_RGB() {};
		Pixel_RGB(std::byte red, std::byte green, std::byte blue) :
			red(red), green(green), blue(blue) {  };
	};

	struct Pixel_RGBA {
		std::byte red, green, blue, alpha;

		Pixel_RGBA() {};
		Pixel_RGBA(std::byte red, std::byte green, std::byte blue, std::byte alpha) :
			red(red), green(green), blue(blue), alpha(alpha) {  };
	};

	struct DataDescription {
		DataDescription(uint32_t channels = 4,
			uint32_t width = 0, uint32_t height = 0,
			uint32_t depth = 1, uint32_t layers = 1) :
			channels(channels),
			width(width), height(height), depth(depth), layers(layers) {
			pixelCount = width * height*depth*layers;
		}

		uint32_t channels = 4; //bytes per pixel (shouldn't be more than 4

		uint32_t width = 0, height = 0, depth = 1, layers = 1;

		uint32_t pixelCount = 0;
	};

	enum class LayoutType {
		single1D,
		single2D,
		single3D,
		array1D,
		array2D,
		array3D,
		cubemap2D,
	};

	enum class FormatType {
		null, //not given...
		png,
		jpg,
	};

	enum class ChannelType
	{
		grey, //STBI_grey = 1,
		grey_alpha, //STBI_grey_alpha = 2,
		rgb, //STBI_rgb = 3,
		rgba, //STBI_rgb_alpha = 4
	};

	struct FileDescription {
		LayoutType layout;
		FormatType format;
		ChannelType channel;
		std::string fileName;

		FileDescription(LayoutType layout,
			FormatType format, ChannelType channel,
			std::string fileName) :
			layout(layout), format(format),
			channel(channel), fileName(fileName) {}
	};

	class TexData {
	public:
		TexData(FileDescription file, DataDescription data);

	private:
		std::vector<std::byte> data;

	};

	class Resource {
	public:
		Resource(TexID id, FileDescription fileDesc,
			DataDescription dataDesc) :
			id(id), fileDescription(fileDesc), dataDescription(dataDesc) {}

		nlohmann::json to_json();

		TexID id;
		FileDescription fileDescription;
		DataDescription dataDescription;
		TexData* dataPtr = nullptr;

	};



	// class Texture {
	// public:
	// 	uint32_t width = 0, height = 0;
	// 	VkDeviceSize texImageSize = 0;

	// 	std::vector<RGBA_pixel> pixels;

	// 	Texture(uint32_t width, uint32_t height);

	// };

	// class TextureArray {
	// public:
	// 	uint32_t width = 0, height = 0;
	// 	uint32_t layerCount = 1;
	// 	VkDeviceSize texImageSize = 0;
	// 	VkDeviceSize texImageSizePerTex = 0;

	// 	std::vector<RGBA_pixel> pixels;

	// 	TextureArray(uint32_t width, uint32_t height, uint32_t layerCount = 1);
	// };

	// class CubeMap {
	// public:

	// 	uint32_t width = 0, height = 0;
	// 	uint32_t layerCount = 6;
	// 	VkDeviceSize texImageSize = 0;
	// 	VkDeviceSize texImageSizePerTex = 0;

	// 	std::vector<RGBA_pixel> pixels;

	// 	CubeMap(uint32_t width, uint32_t height);

	// };



	class Manager {
	public:
		Manager();
		~Manager();

		void LoadTextureList();
		void SaveTextureList();

		void LoadTexture(int id);

		Resource GetTexResourceByID(int id);
		// std::shared_ptr<Texture> loadTextureFromFileRGBA(std::string filename);
		// std::shared_ptr<Texture> loadTextureFromFileGreyOnly(std::string filename);
		// //void loadFromNoiseUtilImage(utils::Image* image);
		// std::shared_ptr<Texture> loadTextureFromGreyscalePixelData(int width, int height, float* pixels);
		// std::shared_ptr<Texture> loadTextureFromRGBAPixelData(int width, int height, std::vector<RGBA_pixel>* pixels);

		// std::shared_ptr<TextureArray> loadTextureArrayFromFile(std::string path, std::vector<std::string> filenames);

		// std::shared_ptr<CubeMap> loadCubeMapFromFile(std::string filename, std::string fileExt);

	private:
		std::mutex lock;
		std::vector<Resource> textureResources;
		std::vector<std::unique_ptr<TexData>> textureData;

		// std::vector<std::shared_ptr<Texture>> textureHandles;

		// std::shared_ptr<Texture> loadTextureFromFile(std::string filename, int imgType);
		// std::shared_ptr<Texture> loadTextureFromPixelData(int width, int height); //doesn't copy pixels, but it sets up everthing else

		//std::shared_ptr<Texture> errorImage;
		//std::vector<RGBA_pixel> errorImageData;
	};

}