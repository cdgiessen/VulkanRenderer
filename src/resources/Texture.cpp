#include "Texture.h"

#include "../../third-party/stb_image/stb_image.h"

#include "../core/CoreTools.h"
#include "../core/Logger.h"

#include <json.hpp>

namespace Resources::Texture {


nlohmann::json to_json(DataDescription& desc){
	nlohmann::json j;
	j["channels"] = desc.channels;
	j["width"] = desc.width;
	j["height"] = desc.height;
	j["depth"] = desc.depth;
	j["layers"] = desc.layers;
	j["pixelCount"] = desc.pixelCount;
	return j;
}
DataDescription from_json_TexDataDesc(nlohmann::json j){
	return DataDescription(j["channels"], j["width"], j["height"],
		j["depth"], j["layers"]);
}

std::string to_string(LayoutType& layout){
	switch(layout){
		case(LayoutType::single1D): return "single1D"; break;
		case(LayoutType::single2D): return "single2D"; break;
		case(LayoutType::single3D): return "single3D"; break;
		case(LayoutType::array1D): return "array1D"; break;
		case(LayoutType::array2D): return "array2D"; break;
		case(LayoutType::array3D): return "array3D"; break;
		case(LayoutType::cubemap2D): return "cubemap2D"; break;
	}
}
LayoutType from_string_TexLayout(std::string s){
	if(s == "single1D") return LayoutType::single1D;
	if(s == "single2D") return LayoutType::single2D;
	if(s == "single3D") return LayoutType::single3D;
	if(s == "array1D") return LayoutType::array1D;
	if(s == "array2D") return LayoutType::array2D;
	if(s == "array3D") return LayoutType::array3D;
	if(s == "cubemap2D") return LayoutType::cubemap2D;
	return LayoutType::single2D; //good default
}

std::string to_string(FormatType& format){
	switch(format){
		case(FormatType::png): return "png"; break;
		case(FormatType::jpg): return "jpg"; break;
	}
}
FormatType from_string_TexFormat(std::string s){
	if(s == "png") return FormatType::png;
	if(s == "jpg") return FormatType::jpg;
	return FormatType::null;
}

std::string to_string(ChannelType& channel){
	switch(channel){
		case(ChannelType::grey): return "grey"; break;
		case(ChannelType::grey_alpha): return "grey_alpha"; break;
		case(ChannelType::rgb): return "rgb"; break;
		case(ChannelType::rgba): return "rgba"; break;
	}
}
ChannelType from_string_TexChannel(std::string s){
	if(s == "grey") return ChannelType::grey;
	if(s == "grey_alpha") return ChannelType::grey_alpha;
	if(s == "rgb") return ChannelType::rgb;
	return ChannelType::rgba; //good default

}

nlohmann::json to_json(FileDescription& tfd){
	nlohmann::json j;
	j["layout"] = to_string(tfd.layout);
	j["format"] = to_string(tfd.format);
	j["channels"] = to_string(tfd.channel);
	j["fileName"] = tfd.fileName;
	return j;
}
FileDescription from_json_TexFileDesc(nlohmann::json j){
	return FileDescription(j["laytout"], j["format"], j["channel"], j["fileName"]);
}

nlohmann::json Resource::to_json(){
	nlohmann::json j;
	j["id"] = id;
	j["fileDescription"] = to_json(fileDescription);
	j["dataDescription"] = to_json(dataDescription);
	return j;
}

Resource from_json_TexResource(nlohmann::json j){
	return Resource(j["id"], from_json_TexFileDesc(j["fileDescription"],
		from_json_TexDataDesc(j["dataDescription"]);
}

// Texture::Texture(uint32_t width, uint32_t height)
// 	:width(width), height(height)
// {
// 	pixels.resize(width*height);
// 	texImageSize = width * height * 4;
// };

// TextureArray::TextureArray(uint32_t width, uint32_t height, uint32_t layerCount)
// 	:width(width), height(height), layerCount(layerCount)
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

Manager::Manager()
{
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

	FileDescription simpleImage(LayoutType::single2D, FormatType::jpg, ChannelType::rgba);
	DataDescription exampleData(4, 1024, 1024, 1, 1);
	TextureR tex(0, simpleImage, exampleData);
	textureMap[0] = tex;
	SaveTextureList();
}

Manager::~Manager()
{
}

void Manager::LoadTextureList() {
	nlohmann::json j;
	
	if (fileExists("assets/TextureList.json")) {
		//try {
			std::ifstream inFile("assets/TextureList.json");
			inFile >> j;
		//}
		//catch (std::parse_error e) {
			Log::Debug << "Texture List was invalid json, creating new one\n";
			Log::Debug << "Json error: " << e.what() << "\n";
			SaveTextureList();
		//}
	}
	else {
		Log::Debug << "Texture List file didn't exist, creating one";
		SaveTextureList();
	}

	try{
		int num_texs = j["num_textures"];
		for (int i = 0; i < num_texs; i++) {	
			textureMap = from_json_TexResource(j[i]); 
		}
	}
	catch(std::runtime_error e){
		Log::Debug << "Error loading texture list " << e.what() << "\n";
	}
}

void Manager::SaveTextureList() {
	nlohmann::json j;
	int num_texs = textureMap.size();
	int i = 0;
	for (auto const& [key, val] : textureMap) {	
		j[i] = to_json(val);
		i++;
	}

	std::ofstream outFile("assets/TextureList.json");
	outFile << std::setw(4) << j;
	outFile.close();
}

void Manager::LoadTexture(int id){
	auto resource = GetTexResourceByID(id); 

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels;
	pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, type);

	if (pixels == nullptr) {
		Log::Error << "Image failed to load! Was the name correct?\n";
		//return nullptr;
	}

	stbi_image_free(pixels);
}

Resource Manager::GetTexResourceByID(int id){
	std::lock_guard<std::mutex> lk(lock);
	for(auto& r : textureResources){
		if(r.id == id)
			return r;
	}
	throw std::runtime_error("Tried to access texture that doesn't exists! Id = " + id);
}

// std::shared_ptr<Texture> Manager::loadTextureFromFile(std::string filename, int imgType) {

// 	int texWidth, texHeight, texChannels;
// 	stbi_uc* pixels;
// 	pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, imgType);

// 	if (pixels == nullptr) {
// 		Log::Error << "Image failed to load! Was the name correct?\n";
// 		return nullptr;
// 	}

// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(texWidth, texHeight);

// 	std::memcpy(tex->pixels.data(), pixels, texWidth * texHeight * 4);

// 	stbi_image_free(pixels);
// 	std::lock_guard<std::mutex> lg(lock);
// 	textureHandles.push_back(tex);
// 	return tex;
// }

// std::shared_ptr<Texture> Manager::loadTextureFromFileRGBA(std::string filename) {
// 	return loadTextureFromFile(filename, STBI_rgb_alpha);
// };
// std::shared_ptr<Texture> Manager::loadTextureFromFileGreyOnly(std::string filename) {
// 	return loadTextureFromFile(filename, STBI_grey);
// };

// std::shared_ptr<Texture> Manager::loadTextureFromGreyscalePixelData(int width, int height, float* in_pixels) {
// 	if (width < 0 || height < 0) {
// 		Log::Error << "Can't have negative dimentions!\n";
// 		return nullptr;
// 	}

// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);

// 	if (in_pixels == nullptr) {
// 		Log::Debug << "Noise Utils Image Null, Cannot load null image!\n";
// 		return nullptr;
// 	}


// 	for (int i = 0; i < width; i++)
// 	{
// 		for (int j = 0; j < height; j++)
// 		{
// 			tex->pixels[i * width + j] = RGBA_pixel(
// 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
// 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
// 				static_cast<stbi_uc>(in_pixels[i * tex->height + j] * 128 + 128),
// 				1);
// 		}
// 	}


// 	std::lock_guard<std::mutex> lg(lock);
// 	textureHandles.push_back(tex);

// 	return tex;
// }

// std::shared_ptr<Texture> Manager::loadTextureFromRGBAPixelData(int width, int height, std::vector<RGBA_pixel>* in_pixels) {
// 	if (width < 0 || height < 0) {
// 		Log::Error << "Can't have negative dimentions!\n";
// 		return nullptr;
// 	}

// 	if (in_pixels == nullptr) {
// 		Log::Debug << "Noise Utils Image Null, Cannot load null image!\n";
// 		return nullptr;
// 	}

// 	std::shared_ptr<Texture> tex = std::make_shared<Texture>(width, height);
// 	tex->pixels = *in_pixels;
// 	//std::memcpy(tex->pixels.data(), in_pixels->data(), tex->texImageSize);	

// 	return tex;
// }


// std::shared_ptr<TextureArray> Manager::loadTextureArrayFromFile(std::string path, std::vector<std::string> filenames) {
// 	std::shared_ptr<TextureArray> texArray;
// 	std::vector<std::shared_ptr<Texture>> textures;
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
// 		texArray = std::make_shared < TextureArray>(width, height, textures.size());

// 		for (int l = 0; l < textures.size(); l++)
// 		{
// 			std::memcpy(texArray->pixels.data() + l * width*height, textures.at(l)->pixels.data(), width*height * 4);
// 		}
// 	}
// 	else {
// 		throw std::runtime_error("Texture array not all same size!");
// 	}

// 	return texArray;
// }


// std::shared_ptr<CubeMap> Manager::loadCubeMapFromFile(std::string filename, std::string fileExt) {
// 	std::shared_ptr<CubeMap> cubeMap;
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
// 			std::memcpy(cubeMap->pixels.data() + l * width * height, faces.at(l)->pixels.data(), width * height * 4);
// 		}

// 	}
// 	else {
// 		throw std::runtime_error("Cubemap faces not all same size!");
// 	}

// 	return cubeMap;
// };


}