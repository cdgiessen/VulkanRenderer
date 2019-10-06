#pragma once

#include <atomic>
#include <cstddef>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/JobSystem.h"

namespace Resource::Texture
{

using TexID = int;

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

class TexResource
{
	public:
	TexResource (){};
	TexResource (TexID id, std::string name, LayoutType layout, ChannelType channels, FormatType format, DataDescription dataDesc)
	: id (id), name (name), layout (layout), channels (channels), fileFormatType (format), description (dataDesc)
	{
		data.resize (description.channels * description.pixelCount);
	}

	TexID id;
	std::string name;
	LayoutType layout;
	ChannelType channels;
	FormatType fileFormatType;
	DataDescription description;

	std::vector<std::byte> data;
};

class Manager
{
	public:
	Manager (job::TaskManager& task_manager);
	~Manager ();

	void LoadTextureList ();
	void SaveTextureList ();

	TexID GetNextFreeTexID ();

	void LoadTextureFromFile (TexID texRes);

	TexID GetTexIDByName (std::string s);
	TexResource& GetTexResourceByID (int id);


	private:
	job::TaskManager& task_manager;

	std::atomic_int id_counter = 0;

	std::mutex resource_lock;
	std::unordered_map<TexID, TexResource> textureResources;
};

} // namespace Resource::Texture