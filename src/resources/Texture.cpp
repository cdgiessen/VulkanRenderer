#include "Texture.h"

#include <fstream>
#include <iomanip>

#include "DTex/DTex.hpp"

#include "stb_image/stb_image.h"

#include "core/CoreTools.h"
#include "core/JobSystem.h"
#include "core/Logger.h"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>


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

nlohmann::json TexResource::to_json () const
{
	nlohmann::json j;
	j["id"] = id;
	switch (layout)
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
	switch (fileFormatType)
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
	switch (channels)
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
TexResource from_json_TexResource (nlohmann::json j)
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

	uint32_t channels = j["dataDescription"]["channels"];
	uint32_t width = j["dataDescription"]["width"];
	uint32_t height = j["dataDescription"]["height"];
	uint32_t depth = j["dataDescription"]["depth"];
	uint32_t layers = j["dataDescription"]["layers"];
	DataDescription dataDesc (channels, width, height, depth, layers);

	return TexResource (id, name, layout, channelType, format, dataDesc);

	// return std::tuple<TexID, std::string, LayoutType,
	//	ChannelType, FormatType,DataDescription>(id, fileName, layout, channel,
	// format, dataDesc);
}

std::byte* TexResource::GetByteDataPtr ()
{
	if (dataPtr != nullptr) return dataPtr->GetDataPtr ();
	return nullptr;
}

TexData::TexData (DataDescription inData)
{
	// size_t size = inData.channels * inData.pixelCount;
	size_t size = 4 * inData.pixelCount;
	data.resize (size);
}

std::byte* TexData::GetDataPtr () { return data.data (); }

void TexResource::SetDataPtr (TexData* texData) { dataPtr = texData; }


Manager::Manager () { LoadTextureList (); }

Manager::~Manager () {}

TexID Manager::GetNextFreeTexID () { return id_counter++; }

void Manager::LoadTextureList ()
{
	nlohmann::json j;

	if (fileExists ("assets/TextureList.json"))
	{
		std::ifstream inFile ("assets/TextureList.json");
		try
		{
			inFile >> j;
		}
		catch (nlohmann::json::parse_error& e)
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
		Log.Debug (fmt::format ("Loaded {} textuers\n", textureResources.size ()));
		int count = 0;

		auto signal = std::make_shared<job::TaskSignal> ();
		std::vector<job::Task> tasks;
		for (nlohmann::json::iterator it = j.begin (); it != j.end (); ++it)
		{
			auto tex = from_json_TexResource (*it);
			textureResources[tex.id] = tex;
			tasks.push_back (job::Task (signal, [=] { LoadTextureFromFile (tex.id); }));
			count++;
		}
		id_counter = count;
		taskManager.Submit (tasks, job::TaskType::currentFrame);
		signal->Wait ();
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log.Debug (fmt::format ("Error loading texture list {}\n", e.what ()));
	}
}

void Manager::SaveTextureList ()
{
	nlohmann::json j;
	// j["num_texs"] = textureResources.size();
	for (auto const& [key, val] : textureResources)
	{
		j[key] = val.to_json ();
	}
	std::ofstream outFile ("assets/TextureList.json");
	try
	{
		outFile << std::setw (4) << j;
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log.Debug (fmt::format ("{}\n", e.what ()));
	}
	outFile.close ();
}

void Manager::LoadTextureFromFile (TexID id)
{
	auto& texRes = GetTexResourceByID (id);
	auto texData = std::make_unique<TexData> (texRes.dataDescription);
	texRes.SetDataPtr (texData.get ());

	int desiredChannels = texRes.dataDescription.channels;

	std::vector<std::string> paths;
	for (int i = 0; i < texRes.dataDescription.layers; i++)
	{
		std::string path;
		if (texRes.dataDescription.layers != 1)
		{
			path = std::string ("assets/textures/") + texRes.name + std::string ("_") +
			       std::to_string (i) + std::string (".") + formatTypeToString (texRes.fileFormatType);
		}
		else
		{
			path = std::string ("assets/textures/") + texRes.name + std::string (".") +
			       formatTypeToString (texRes.fileFormatType);
		}

		paths.push_back (path);

		LoadTextureDTex (path);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels;
		pixels = stbi_load (path.c_str (), &texWidth, &texHeight, &texChannels, 4);

		if (pixels == nullptr)
		{
			Log.Error (fmt::format ("Image {} failed to load!\n", path.c_str ()));
		}
		// else if (desiredChannels != texChannels) {
		//	Log::Error << "Image couldn't load desired channel of " << desiredChannels
		//		<< " but only " << texChannels << " channels\n";
		//}
		else
		{
			std::memcpy (texData->GetDataPtr () + i * texWidth * texHeight * 4,
			    pixels,
			    sizeof (std::byte) * texWidth * texHeight * 4);


			stbi_image_free (pixels);
		}
	}

	std::lock_guard<std::mutex> lg (data_lock);
	textureData.push_back (std::move (texData));

	Log.Debug (fmt::format ("Tex {}\n", id));
}

void Manager::LoadTextureDTex (std::string path)
{
	auto loadResult = DTex::LoadFromFile (path);
	if (loadResult.GetResultInfo () != DTex::ResultInfo::Success)
	{
		Log.Error (fmt::format ("Failed to load file '{}'\n", path));
		Log.Error (fmt::format ("Detailed error: '{}'\n", loadResult.GetErrorMessage ()));
		return;
	}

	Log.Debug (fmt::format ("Loaded image at {} \n", path));

	auto& texDoc = loadResult.GetValue ();

	auto bDims = texDoc.GetBaseDimensions ();
	Log.Debug (fmt::format ("Base dims are x:{}, y:{}, z:{}\n", bDims.width, bDims.height, bDims.depth));

	if (texDoc.GetPixelFormat () == DTex::PixelFormat::BC7)
		Log.Debug ("Format is VK_FORMAT_BC7_UNORM_BLOCK.\n");
	else if (texDoc.GetPixelFormat () == DTex::PixelFormat::RGB_8)
		Log.Debug ("Image format is VK_FORMAT_R8G8B8.\n");
	else if (texDoc.GetPixelFormat () == DTex::PixelFormat::RGBA_8)
		Log.Debug ("Image format is VK_FORMAT_R8G8B8A8.\n");
	if (texDoc.GetTextureType () == DTex::TextureType::Texture2D)
		Log.Debug ("ImageType is VK_IMAGE_TYPE_2D.\n");
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
