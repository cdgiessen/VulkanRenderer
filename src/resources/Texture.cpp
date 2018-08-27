#include "Texture.h"

#include "../../third-party/stb_image/stb_image.h"

#include "../core/CoreTools.h"
#include "../core/Logger.h"

#include <json.hpp>

namespace Resource::Texture {

	// DataDescription from_json_TexDataDesc(nlohmann::json j){
	// 	return DataDescription(j["channels"], j["width"], j["height"],
	// 		j["depth"], j["layers"]);
	// }
	// LayoutType from_string_TexLayout(std::string s){
	// 	if(s == "single1D") return LayoutType::single1D;
	// 	if(s == "single2D") return LayoutType::single2D;
	// 	if(s == "single3D") return LayoutType::single3D;
	// 	if(s == "array1D") return LayoutType::array1D;
	// 	if(s == "array2D") return LayoutType::array2D;
	// 	if(s == "array3D") return LayoutType::array3D;
	// 	if(s == "cubemap2D") return LayoutType::cubemap2D;
	// 	return LayoutType::single2D; //good default
	// }

	std::string formatTypeToString(FormatType type) {
		switch (type) {
		case (FormatType::jpg):
			return "jpg";
		case (FormatType::png):
			return "png";
		}
		return "";
	}

	// FormatType from_string_TexFormat(std::string s){
	// 	if(s == "png") return FormatType::png;
	// 	if(s == "jpg") return FormatType::jpg;
	// 	return FormatType::null;
	// }

	// ChannelType from_string_TexChannel(std::string s){
	// 	if(s == "grey") return ChannelType::grey;
	// 	if(s == "grey_alpha") return ChannelType::grey_alpha;
	// 	if(s == "rgb") return ChannelType::rgb;
	// 	return ChannelType::rgba; //good default

	// }

	// FileDescription from_json_TexFileDesc(nlohmann::json j){
	// 	return FileDescription(j["laytout"], j["format"], j["channel"],
	// j["fileName"]);
	// }

	nlohmann::json TexResource::to_json() const {
		nlohmann::json j;
		j["id"] = id;
		switch (layout) {
		case (LayoutType::single1D):
			j["layout"] = "single1D";
			break;
		case (LayoutType::single2D):
			j["layout"] = "single2D";
			break;
		case (LayoutType::single3D):
			j["layout"] = "single3D";
			break;
		case (LayoutType::array1D):
			j["layout"] = "array1D";
			break;
		case (LayoutType::array2D):
			j["layout"] = "array2D";
			break;
		case (LayoutType::array3D):
			j["layout"] = "array3D";
			break;
		case (LayoutType::cubemap2D):
			j["layout"] = "cubemap2D";
			break;
		}
		switch (fileFormatType) {
		case (FormatType::png):
			j["format"] = "png";
			break;
		case (FormatType::jpg):
			j["format"] = "jpg";
			break;
		}
		switch (channels) {
		case (ChannelType::grey):
			j["channels"] = "grey";
			break;
		case (ChannelType::grey_alpha):
			j["channels"] = "grey_alpha";
			break;
		case (ChannelType::rgb):
			j["channels"] = "rgb";
			break;
		case (ChannelType::rgba):
			j["channels"] = "rgba";
			break;
		}
		j["name"] = name;
		j["dataDescription"]["channels"] = dataDescription.channels;
		j["dataDescription"]["width"] = dataDescription.width;
		j["dataDescription"]["height"] = dataDescription.height;
		j["dataDescription"]["depth"] = dataDescription.depth;
		j["dataDescription"]["layers"] = dataDescription.layers;
		j["dataDescription"]["pixelCount"] = dataDescription.pixelCount;
		return j;
	}

	// std::tuple<TexID,std::string, LayoutType,
	//		ChannelType, FormatType,DataDescription>
	TexResource from_json_TexResource(nlohmann::json j) {
		int id = j["id"];

		LayoutType layout;
		if (j["layout"] == "single1D")
			layout = LayoutType::single1D;
		else if (j["layout"] == "single2D")
			layout = LayoutType::single2D;
		else if (j["layout"] == "single3D")
			layout = LayoutType::single3D;
		else if (j["layout"] == "array1D")
			layout = LayoutType::array1D;
		else if (j["layout"] == "array2D")
			layout = LayoutType::array2D;
		else if (j["layout"] == "array3D")
			layout = LayoutType::array3D;
		else if (j["layout"] == "cubemap2D")
			layout = LayoutType::cubemap2D;
		FormatType format;
		if (j["format"] == "png")
			format = FormatType::png;
		else if (j["format"] == "jpg")
			format = FormatType::jpg;
		ChannelType channelType;
		if (j["channels"] == "grey")
			channelType = ChannelType::grey;
		else if (j["channels"] == "grey_alpha")
			channelType = ChannelType::grey_alpha;
		else if (j["channels"] == "rgb")
			channelType = ChannelType::rgb;
		else
			channelType = ChannelType::rgba;
		std::string name = j["name"];

		uint32_t channels = j["dataDescription"]["channels"];
		uint32_t width = j["dataDescription"]["width"];
		uint32_t height = j["dataDescription"]["height"];
		uint32_t depth = j["dataDescription"]["depth"];
		uint32_t layers = j["dataDescription"]["layers"];
		DataDescription dataDesc(channels, width, height, depth, layers);

		return TexResource(id, name, layout, channelType, format, dataDesc);

		// return std::tuple<TexID, std::string, LayoutType,
		//	ChannelType, FormatType,DataDescription>(id, fileName, layout, channel,
		//format, dataDesc);
	}

	std::byte *TexResource::GetByteDataPtr() {
		if (dataPtr != nullptr)
			return dataPtr->GetDataPtr();
		return nullptr;
	}

	TexData::TexData(DataDescription inData) {
		//size_t size = inData.channels * inData.pixelCount;
		size_t size = 4 * inData.pixelCount;
		data.resize(size);
	}

	std::byte *TexData::GetDataPtr() { return data.data(); }

	void TexResource::SetDataPtr(TexData *texData) { dataPtr = texData; }

	// Texture::Texture(uint32_t width, uint32_t height)
	// 	:width(width), height(height)
	// {
	// 	pixels.resize(width*height);
	// 	texImageSize = width * height * 4;
	// };

	// TextureArray::TextureArray(uint32_t width, uint32_t height, uint32_t
	// layerCount) 	:width(width), height(height), layerCount(layerCount)
	// {
	// 	pixels.resize(width*height*layerCount);
	// 	texImageSize = width * height * layerCount * 4;
	// 	texImageSizePerTex = width * height * 4;
	// }

	// CubeMap::CubeMap(uint32_t width, uint32_t height) :
	// 	width(width), height(height)
	// {
	// 	pixels.resize(width*height * 6);
	// 	texImageSize = width * height * 6 * 4;
	// 	texImageSizePerTex = width * height * 4;
	// }

	Manager::Manager() {
		// errorImage = std::make_shared<Texture>(64, 64);
		// std::vector<Pixel_RGBA> pink(64); //{ { RGBA_pixel(255, 0, 255, 255)} };
		// std::vector<Pixel_RGBA> black(64); //= { { RGBA_pixel(0, 0, 0, 255)} };
		// for (int i = 0; i < 64; i++)
		// {
		// pink[i] = Pixel_RGBA(255, 0, 255, 255);
		// black[i] = Pixel_RGBA(0, 0, 0, 255);
		// }

		// for (int i = 0; i < 8; i++)
		// {
		// for (int j = 0; j < 8; j++)
		// {
		// if ((i + j) % 2 == 0) {
		// std::memcpy(&errorImage->pixels[(i * 8 + j) * 8], pink.data(), 64);
		// }
		// }
		// }

		// FileDescription simpleImage(LayoutType::single2D, FormatType::jpg,
		// ChannelType::rgba); DataDescription exampleData(4, 1024, 1024, 1, 1);
		// TextureR tex(0, simpleImage, exampleData);
		// SaveTextureList();

		LoadTextureList();
	}

	Manager::~Manager() {}

	TexID Manager::GetNextFreeTexID() { return id_counter++; }

	void Manager::LoadTextureList() {
		nlohmann::json j;

		if (fileExists("assets/TextureList.json")) {
			std::ifstream inFile("assets/TextureList.json");
			try {
				inFile >> j;
			}
			catch (nlohmann::json::parse_error &e) {
				Log::Debug << "Texture List was invalid json, creating new one\n";
				Log::Debug << "Json error: " << std::string(e.what()) << "\n";
				SaveTextureList();
			}
		}
		else {
			Log::Debug << "Texture List file didn't exist, creating one";
			SaveTextureList();
		}

		try {
			int count = 0;
			for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
				auto tex = from_json_TexResource(*it);
				textureResources[tex.id] = tex;
				LoadTextureFromFile(tex.id);
				count++;
			}
			id_counter = count;
			Log::Debug << textureResources.size() << " textures loaded\n";
			for (auto const&[key, val] : textureResources) {
				Log::Debug << "Tex " << key << " with name " << val.name
					<< " dimensions width " << val.dataDescription.width
					<< " height " << val.dataDescription.height << "\n";
			}
		}
		catch (nlohmann::json::parse_error &e) {
			Log::Debug << "Error loading texture list " << e.what() << "\n";
		}
	}

	void Manager::SaveTextureList() {
		nlohmann::json j;
		//j["num_texs"] = textureResources.size();
		for (auto const &[key, val] : textureResources) {
			j[key] = val.to_json();
		}
		std::ofstream outFile("assets/TextureList.json");
		try {
			outFile << std::setw(4) << j;
		}
		catch (nlohmann::json::parse_error &e)
		{
			Log::Error << e.what() << "\n";
		}
		outFile.close();
	}

	void Manager::LoadTextureFromFile(TexID id) {

		auto &texRes = textureResources.at(id);
		textureData.push_back(std::make_unique<TexData>(texRes.dataDescription));
		texRes.SetDataPtr(&(*textureData.back()));

		int desiredChannels = texRes.dataDescription.channels;

		for (int i = 0; i < texRes.dataDescription.layers; i++) {
			std::string path;
			if (texRes.dataDescription.layers != 1) {
				path = std::string("assets/textures/") + texRes.name + std::string("_") +
					std::to_string(i) + std::string(".") +
					formatTypeToString(texRes.fileFormatType);

			}
			else {
				path = std::string("assets/textures/") + texRes.name + std::string(".") +
					formatTypeToString(texRes.fileFormatType);
			}

			if (path.c_str() == "") {
				Log::Error << "Path not set!\n";
			}
			int texWidth, texHeight, texChannels;
			stbi_uc *pixels;
			pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, 4);

			if (pixels == nullptr) {

				Log::Error << "Image failed to load! Was the name correct? "
					<< path.c_str() << "\n";

			}
			//else if (desiredChannels != texChannels) {
			//	Log::Error << "Image couldn't load desired channel of " << desiredChannels
			//		<< " but only " << texChannels << " channels\n";
			//}
			else {
				std::memcpy(textureData.back()->GetDataPtr() +
					i * texWidth * texHeight * 4,
					pixels,
					sizeof(std::byte) * texWidth * texHeight * 4);


				stbi_image_free(pixels);
			}
		}
	}

	// void Manager::LoadTextureFromDataPtr(std::byte* data){
	// 	auto& texRes = textureResources.at(id);
	// 	textureData.push_back(std::make_unique<TexData>(
	// 		texRes.dataDescription));
	// 	texRes.SetDataPtr(&(*textureData.back()));

	// 	int size = texRes.dataDescription.pixelCount *
	// texRes.dataDescription.channels;

	// 	std::memcpy(textureData.back()->GetDataPtr(), data, sizeof(std::byte) *
	// size);

	// }

	TexID CreateNewTextureFromByteArray(int width, int height, std::byte *data) {
		return 0;
	}

	TexResource Manager::GetTexResourceByID(TexID id) {
		std::lock_guard<std::mutex> lk(lock);
		return textureResources.at(id);
	}

	TexID Manager::GetTexIDByName(std::string s) {
		std::lock_guard<std::mutex> lk(lock);
		for (auto const &[key, val] : textureResources) {
			if (val.name == s)
				return key;
		}
		throw std::runtime_error("No matching texture found with name " + s);
	}

	// std::shared_ptr<Texture> Manager::loadTextureFromFile(std::string filename,
	// int imgType) {

	// 	int texWidth, texHeight, texChannels;
	// 	stbi_uc* pixels;
	// 	pixels = stbi_load(filename.c_str(), &texWidth, &texHeight,
	// &texChannels, imgType);

	// 	if (pixels == nullptr) {
	// 		Log::Error << "Image failed to load! Was the name correct?\n";
	// 		return nullptr;
	// 	}

	// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(texWidth,
	// texHeight);

	// 	std::memcpy(tex->pixels.data(), pixels, texWidth * texHeight * 4);

	// 	stbi_image_free(pixels);
	// 	std::lock_guard<std::mutex> lg(lock);
	// 	textureHandles.push_back(tex);
	// 	return tex;
	// }

	// std::shared_ptr<Texture> Manager::loadTextureFromFileRGBA(std::string
	// filename) { 	return loadTextureFromFile(filename, STBI_rgb_alpha);
	// };
	// std::shared_ptr<Texture> Manager::loadTextureFromFileGreyOnly(std::string
	// filename) { 	return loadTextureFromFile(filename, STBI_grey);
	// };

	// std::shared_ptr<Texture> Manager::loadTextureFromGreyscalePixelData(int
	// width, int height, float* in_pixels) { 	if (width < 0 || height < 0) {
	// 		Log::Error << "Can't have negative dimentions!\n";
	// 		return nullptr;
	// 	}

	// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);

	// 	if (in_pixels == nullptr) {
	// 		Log::Debug << "Noise Utils Image Null, Cannot load null
	// image!\n"; 		return nullptr;
	// 	}

	// 	for (int i = 0; i < width; i++)
	// 	{
	// 		for (int j = 0; j < height; j++)
	// 		{
	// 			tex->pixels[i * width + j] = RGBA_pixel(
	// 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128
	// + 128), 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
	// 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128
	// + 128), 				1);
	// 		}
	// 	}

	// 	std::lock_guard<std::mutex> lg(lock);
	// 	textureHandles.push_back(tex);

	// 	return tex;
	// }

	// std::shared_ptr<Texture> Manager::loadTextureFromRGBAPixelData(int width, int
	// height, std::vector<RGBA_pixel>* in_pixels) { 	if (width < 0 || height < 0) {
	// 		Log::Error << "Can't have negative dimentions!\n";
	// 		return nullptr;
	// 	}

	// 	if (in_pixels == nullptr) {
	// 		Log::Debug << "Noise Utils Image Null, Cannot load null
	// image!\n"; 		return nullptr;
	// 	}

	// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);
	// 	tex->pixels = *in_pixels;
	// 	//std::memcpy(tex->pixels.data(), in_pixels->data(), tex->texImageSize);

	// 	return tex;
	// }

	// std::shared_ptr<TextureArray> Manager::loadTextureArrayFromFile(std::string
	// path, std::vector<std::string> filenames) { 	std::shared_ptr<TextureArray>
	// texArray; 	std::vector<std::shared_ptr<Texture>> textures;
	// 	textures.reserve(filenames.size());

	// 	for (std::string name : filenames) {
	// 		auto tex = loadTextureFromFileRGBA(path + name);
	// 		if (tex != nullptr) {
	// 			textures.push_back(tex);
	// 		}
	// 	}

	// 	if (textures.size() == 0) {
	// 		Log::Error << "No images to load. Is this intended?\n";
	// 	}

	// 	bool sameSize = true;
	// 	int width = textures.at(0)->width;
	// 	int height = textures.at(0)->height;
	// 	for (auto& tex : textures) {
	// 		if (tex->width != width || tex->height != height)
	// 			sameSize = false;
	// 	}
	// 	if (sameSize) {
	// 		texArray = std::make_shared < TextureArray>(width, height,
	// textures.size());

	// 		for (int l = 0; l < textures.size(); l++)
	// 		{
	// 			std::memcpy(texArray->pixels.data() + l * width*height,
	// textures.at(l)->pixels.data(), width*height * 4);
	// 		}
	// 	}
	// 	else {
	// 		throw std::runtime_error("Texture array not all same size!");
	// 	}

	// 	return texArray;
	// }

	// std::shared_ptr<CubeMap> Manager::loadCubeMapFromFile(std::string filename,
	// std::string fileExt) { 	std::shared_ptr<CubeMap> cubeMap;
	// 	std::vector<std::shared_ptr<Texture>> faces;

	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Front" + fileExt));
	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Back" + fileExt));
	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Top" + fileExt));
	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Bottom" + fileExt));
	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Right" + fileExt));
	// 	faces.push_back(loadTextureFromFileRGBA(filename + "Left" + fileExt));

	// 	bool sameSize = true;
	// 	int width = faces.at(0)->width;
	// 	int height = faces.at(0)->height;
	// 	for (auto& tex : faces) {
	// 		if (tex->width != width || tex->height != height)
	// 			sameSize = false;
	// 	}
	// 	if (sameSize) {
	// 		cubeMap = std::make_shared<CubeMap>(width, height);

	// 		for (int l = 0; l < faces.size(); l++)
	// 		{
	// 			std::memcpy(cubeMap->pixels.data() + l * width * height,
	// faces.at(l)->pixels.data(), width * height * 4);
	// 		}

	// 	}
	// 	else {
	// 		throw std::runtime_error("Cubemap faces not all same size!");
	// 	}

	// 	return cubeMap;
	// };

}
