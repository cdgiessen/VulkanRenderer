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

namespace job
{
class ThreadPool;
}


namespace Resource::Texture
{

using TexID = uint32_t;

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

enum class TextureType
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

struct Dimensions
{
	int width = 0, height = 0, channels = 4;
};

class TexResource
{
	public:
	TexID id;
	std::string name;
	TextureType tex_type;
	std::vector<std::string> paths;
	std::vector<Dimensions> dims;
	std::vector<std::byte> data;
};

class Textures
{
	public:
	Textures (job::ThreadPool& thread_pool);
	~Textures ();

	void load_texture_list ();
	void save_texture_list ();

	TexID get_next_free_tex_id ();

	void load_texture_from_file (TexID texRes);

	TexID get_tex_id_by_name (std::string s);
	TexResource& get_tex_resource_by_id (TexID id);


	private:
	job::ThreadPool& thread_pool;

	std::atomic_int id_counter = 0;

	std::mutex resource_lock;
	std::unordered_map<TexID, TexResource> texture_resources;
};

} // namespace Resource::Texture