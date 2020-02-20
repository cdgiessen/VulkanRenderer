#include "Texture.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "stb/stb_image.h"

#include "core/JobSystem.h"
#include "core/Logger.h"




namespace fs = std::filesystem;

const fs::path texture_path = "assets/textures";
const fs::path texture_cache_path = "assets/textures/cache";


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
	switch (r.tex_type)
	{
		case (TextureType::single1D):
			j["type"] = "single1D";
			break;
		case (TextureType::single2D):
			j["type"] = "single2D";
			break;
		case (TextureType::single3D):
			j["type"] = "single3D";
			break;
		case (TextureType::array1D):
			j["type"] = "array1D";
			break;
		case (TextureType::array2D):
			j["type"] = "array2D";
			break;
		case (TextureType::array3D):
			j["type"] = "array3D";
			break;
		case (TextureType::cubemap2D):
			j["type"] = "cubemap2D";
			break;
		default:
			break;
	}
	int i = 0;
	for (auto& p : r.paths)
	{
		j["paths"][i++] = p;
	}

	j["name"] = r.name;
	return j;
}

std::optional<TexResource> from_json_TexResource (nlohmann::json j)
{
	try
	{
		TexID id = j["id"];
		std::string name = j["name"];

		TextureType tex_type;
		if (j["type"] == "single1D")
			tex_type = TextureType::single1D;
		else if (j["type"] == "single2D")
			tex_type = TextureType::single2D;
		else if (j["type"] == "single3D")
			tex_type = TextureType::single3D;
		else if (j["type"] == "array1D")
			tex_type = TextureType::array1D;
		else if (j["type"] == "array2D")
			tex_type = TextureType::array2D;
		else if (j["type"] == "array3D")
			tex_type = TextureType::array3D;
		else if (j["type"] == "cubemap2D")
			tex_type = TextureType::cubemap2D;

		std::vector<std::string> paths;
		for (auto& t : j["paths"])
		{
			paths.push_back (t);
		}

		return { { id, name, tex_type, paths } };
	}
	catch (nlohmann::json::exception& e)
	{
		Log.Error (fmt::format ("failed to parse texture"));
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

	if (fs::exists ("assets/texture_db.json"))
	{
		std::ifstream inFile ("assets/texture_db.json");
		try
		{
			inFile >> j;
		}
		catch (nlohmann::json::exception& e)
		{
			Log.Debug ("Texture db was invalid json, creating a new one");
			Log.Debug (fmt::format ("Json error: {}", e.what ()));
			SaveTextureList ();
		}
	}
	else
	{
		Log.Debug ("Texture db didn't exist, creating one");
		SaveTextureList ();
	}

	try
	{
		Log.Debug (fmt::format ("Loading {} textures", textureResources.size ()));
		int count = 0;

		auto signal = std::make_shared<job::TaskSignal> ();
		std::vector<job::Task> tasks;
		for (nlohmann::json::iterator it = j.begin (); it != j.end (); ++it)
		{
			auto tex = from_json_TexResource (*it);
			if (tex.has_value ())
			{
				TexID id = tex.value ().id;
				textureResources[id] = tex.value ();
				tasks.push_back (job::Task ([id, this] { LoadTextureFromFile (id); }, signal));
				count++;
			}
			else
			{
				Log.Error (fmt::format ("Tex resource is invalid"));
			}
		}
		id_counter = count;
		task_manager.Submit (tasks);
		signal->Wait ();
	}
	catch (nlohmann::json::exception& e)
	{
		Log.Debug (fmt::format ("Error loading texture list {}", e.what ()));
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
		Log.Debug (fmt::format ("{}", e.what ()));
	}
	outFile.close ();
}

void Manager::LoadTextureFromFile (TexID id)
{
	auto& texRes = GetTexResourceByID (id);

	std::vector<std::byte> texData; // = std::vector<std::byte> (texRes.description.pixelCount * 4);

	// int desiredChannels = texRes.dataDescription.channels;
	std::vector<stbi_uc*> pixels_array (texRes.paths.size ());
	texRes.dims.resize (texRes.paths.size ());

	for (size_t i = 0; i < texRes.paths.size (); i++)
	{
		fs::path path = texRes.paths.at (i);
		std::string path_string = "assets/textures/" + path.string ();
		pixels_array.at (i) = stbi_load (path_string.c_str (),
		    &texRes.dims.at (i).width,
		    &texRes.dims.at (i).height,
		    &texRes.dims.at (i).channels,
		    4);
		if (pixels_array.at (i) == nullptr)
		{
			Log.Error (fmt::format ("Image {} failed to load!", path.string ()));
		}
	}

	int size = 0;
	for (size_t i = 0; i < texRes.paths.size (); i++)
	{
		size += texRes.dims.at (i).width * texRes.dims.at (i).height * 4;
	}
	texData.resize (size);

	for (size_t i = 0; i < texRes.paths.size (); i++)
	{
		std::memcpy (texData.data () + i * texRes.dims.at (i).width * texRes.dims.at (i).height * 4,
		    pixels_array.at (i),
		    sizeof (std::byte) * texRes.dims.at (i).width * texRes.dims.at (i).height * 4);

		stbi_image_free (pixels_array.at (i));
	}

	Log.Debug (fmt::format ("Tex {}", id));

	std::lock_guard lg (resource_lock);
	texRes.data = std::move (texData);
}

TexResource& Manager::GetTexResourceByID (TexID id)
{
	std::lock_guard lk (resource_lock);
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
