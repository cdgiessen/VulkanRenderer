#include "Texture.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

#include "stb/stb_image.h"

#include "core/JobSystem.h"
#include "core/Logger.h"

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

const fs::path texture_path = "assets/textures";

namespace Resource::Texture
{

std::string formatTypeToString (FormatType type)
{
	switch (type)
	{
		case (FormatType::jpg):
			return "jpg";
		case (FormatType::png):
			return "png";
		default:
			return "Image type not valid!";
	}
	return "";
}

nlohmann::json to_json (TexResource const& r)
{
	nlohmann::json j;
	j["id"] = r.id;
	switch (r.layout)
	{
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
		default:
			break;
	}
	switch (r.fileFormatType)
	{
		case (FormatType::png):
			j["format"] = "png";
			break;
		case (FormatType::jpg):
			j["format"] = "jpg";
			break;
		default:
			break;
	}
	switch (r.channels)
	{
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
	j["name"] = r.name;
	j["description"]["channels"] = r.description.channels;
	j["description"]["width"] = r.description.width;
	j["description"]["height"] = r.description.height;
	j["description"]["depth"] = r.description.depth;
	j["description"]["layers"] = r.description.layers;
	j["description"]["pixelCount"] = r.description.pixelCount;
	return j;
}

std::optional<TexResource> from_json_TexResource (nlohmann::json j)
{
	try
	{
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

		uint32_t channels = j["description"]["channels"];
		uint32_t width = j["description"]["width"];
		uint32_t height = j["description"]["height"];
		uint32_t depth = j["description"]["depth"];
		uint32_t layers = j["description"]["layers"];
		DataDescription dataDesc (channels, width, height, depth, layers);
		return TexResource (id, name, layout, channelType, format, dataDesc);
	}
	catch (nlohmann::json::exception& e)
	{
		Log.Error (fmt::format ("failed to parse texture\n"));
		return {};
	}
}

Manager::Manager (job::TaskManager& task_manager) : task_manager (task_manager)
{
	LoadTextureList ();
}

Manager::~Manager () {}

TexID Manager::GetNextFreeTexID () { return id_counter++; }

void Manager::LoadTextureList ()
{
	nlohmann::json j;

	if (fs::exists ("assets/TextureList.json"))
	{
		std::ifstream inFile ("assets/TextureList.json");
		try
		{
			inFile >> j;
		}
		catch (nlohmann::json::exception& e)
		{
			Log.Debug ("Texture list was invalid json, creating a new one\n");
			Log.Debug (fmt::format ("Json error: {}\n", e.what ()));
			SaveTextureList ();
		}
	}
	else
	{
		Log.Debug ("Texture list didn't exist, creating one");
		SaveTextureList ();
	}

	try
	{
		Log.Debug (fmt::format ("Loaded {} textures\n", textureResources.size ()));
		int count = 0;

		auto signal = std::make_shared<job::TaskSignal> ();
		std::vector<job::Task> tasks;
		for (nlohmann::json::iterator it = j.begin (); it != j.end (); ++it)
		{
			auto tex = from_json_TexResource (*it);
			if (tex.has_value ())
			{

				textureResources[tex.value ().id] = tex.value ();
				tasks.push_back (job::Task ([=] { LoadTextureFromFile (tex.value ().id); }, signal));
				count++;
			}
			else
			{
				Log.Error ("No Texture Resource\n");
			}
		}
		id_counter = count;
		task_manager.Submit (tasks);
		signal->Wait ();
	}
	catch (nlohmann::json::exception& e)
	{
		Log.Debug (fmt::format ("Error loading texture list {}\n", e.what ()));
	}
}

void Manager::SaveTextureList ()
{
	nlohmann::json j;
	for (auto const& [key, val] : textureResources)
	{
		j[key] = to_json (val);
	}
	std::ofstream outFile ("assets/TextureList.json");
	if (outFile.bad ())
	{
		throw std::runtime_error ("could not save texture list!");
	}
	try
	{
		outFile << std::setw (4) << j;
	}
	catch (nlohmann::json::exception& e)
	{
		Log.Debug (fmt::format ("{}\n", e.what ()));
	}
	outFile.close ();
}

void Manager::LoadTextureFromFile (TexID id)
{
	auto& texRes = GetTexResourceByID (id);
	auto texData = std::make_unique<std::vector<std::byte>> (
	    texRes.description.pixelCount * texRes.description.channels);

	// int desiredChannels = texRes.dataDescription.channels;

	std::vector<fs::path> paths;
	for (size_t i = 0; i < texRes.description.layers; i++)
	{
		fs::path path;
		if (texRes.description.layers != 1)
		{
			path = texture_path / fs::path (texRes.name + "_" + std::to_string (i) + "." +
			                                formatTypeToString (texRes.fileFormatType));
		}
		else
		{
			path = texture_path / fs::path (texRes.name + std::string (".") +
			                                formatTypeToString (texRes.fileFormatType));
		}
		paths.push_back (path);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels;
		pixels = stbi_load (path.c_str (), &texWidth, &texHeight, &texChannels, 4);

		if (pixels == nullptr)
		{
			Log.Error (fmt::format ("Image {} failed to load!\n", path.string ()));
		}
		// else if (desiredChannels != texChannels) {
		//	Log.Error << "Image couldn't load desired channel of " << desiredChannels
		//		<< " but only " << texChannels << " channels\n";
		//}
		else
		{
			std::memcpy (texData.get ()->data () + i * texWidth * texHeight * texChannels,
			    pixels,
			    sizeof (std::byte) * texWidth * texHeight * texChannels);


			stbi_image_free (pixels);
		}
	}

	std::lock_guard<std::mutex> lg (data_lock);
	textureData.push_back (std::move (texData));

	Log.Debug (fmt::format ("Tex {}\n", id));
}

TexID CreateNewTextureFromByteArray (int width, int height, std::byte* data) { return 0; }

TexResource& Manager::GetTexResourceByID (TexID id)
{
	std::lock_guard<std::mutex> lk (resource_lock);
	return textureResources.at (id);
}

TexID Manager::GetTexIDByName (std::string s)
{
	std::lock_guard<std::mutex> lk (resource_lock);
	for (auto const& [key, val] : textureResources)
	{
		if (val.name == s) return key;
	}
	throw std::runtime_error ("No matching texture found with name " + s);
}

} // namespace Resource::Texture
