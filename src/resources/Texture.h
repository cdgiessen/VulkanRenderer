#pragma once

#include <string>

#include <atomic>
#include <cstddef>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <glm/fwd.hpp>

#include <nlohmann/json_fwd.hpp>

namespace Resource::Texture
{

using TexID = int;

struct Pixel_R
{
	std::byte red;

	Pixel_R (){};
	Pixel_R (std::byte red) : red (red){};
};

struct Pixel_RG
{
	std::byte red, green;

	Pixel_RG (){};
	Pixel_RG (std::byte red, std::byte green) : red (red), green (green){};
};

struct Pixel_RGB
{
	std::byte red, green, blue;

	Pixel_RGB (){};
	Pixel_RGB (std::byte red, std::byte green, std::byte blue)
	: red (red), green (green), blue (blue){};
};

struct Pixel_RGBA
{
	std::byte red, green, blue, alpha;

	Pixel_RGBA (){};
	Pixel_RGBA (std::byte red, std::byte green, std::byte blue, std::byte alpha)
	: red (red), green (green), blue (blue), alpha (alpha){};
};

struct DataDescription
{
	DataDescription (
	    uint32_t channels = 4, uint32_t width = 0, uint32_t height = 0, uint32_t depth = 1, uint32_t layers = 1)
	: channels (channels), width (width), height (height), depth (depth), layers (layers)
	{
		pixelCount = width * height * depth * layers;
	}

	uint32_t channels = 4; // bytes per pixel (shouldn't be more than 4

	uint32_t width = 0, height = 0, depth = 1, layers = 1;

	uint32_t pixelCount = 0;
};

enum class LayoutType
{
	single1D,
	single2D,
	single3D,
	array1D,
	array2D,
	array3D,
	cubemap2D,
};

enum class FormatType
{
	null, // not given...
	png,
	jpg,
};

enum class ChannelType
{
	grey,       // STBI_grey = 1,
	grey_alpha, // STBI_grey_alpha = 2,
	rgb,        // STBI_rgb = 3,
	rgba,       // STBI_rgb_alpha = 4
};

// struct FileDescription {
// 	LayoutType layout;
// 	FormatType format;
// 	ChannelType channel;
// 	std::string fileName;

// 	FileDescription(LayoutType layout,
// 		FormatType format, ChannelType channel,
// 		std::string fileName):
// 		layout(layout), format(format),
// 		channel(channel), fileName(fileName) {}
// };

class TexData
{
	public:
	TexData (DataDescription data);
	std::byte* GetDataPtr ();

	private:
	std::vector<std::byte> data;
};


class TexResource
{
	public:
	TexResource (){};
	TexResource (TexID id, std::string name, LayoutType layout, ChannelType channels, FormatType format, DataDescription dataDesc)
	: id (id), name (name), layout (layout), channels (channels), fileFormatType (format), dataDescription (dataDesc)
	{
	}

	nlohmann::json to_json () const;
	void SetDataPtr (TexData* texData);
	std::byte* GetByteDataPtr ();

	TexID id;
	std::string name;
	LayoutType layout;
	ChannelType channels;
	FormatType fileFormatType;
	DataDescription dataDescription;

	private:
	TexData* dataPtr = nullptr;
};

class Manager
{
	public:
	Manager ();
	~Manager ();

	void LoadTextureList ();
	void SaveTextureList ();

	TexID GetNextFreeTexID ();

	void LoadTextureFromFile (TexID id);

	TexID GetTexIDByName (std::string s);
	TexResource GetTexResourceByID (int id);


	private:
	std::atomic_int id_counter = 0;

	std::mutex lock;
	std::unordered_map<TexID, TexResource> textureResources;
	std::vector<std::unique_ptr<TexData>> textureData;
};

} // namespace Resource::Texture