#include "Texture.h"

#include <iomanip>

#include "stb_image/stb_image.h"

#include "core/CoreTools.h"
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
	}
	switch (fileFormatType)
	{
		case (FormatType::png):
			j["format"] = "png";
			break;
		case (FormatType::jpg):
			j["format"] = "jpg";
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
			Log::Debug << "Texture List was invalid json, creating new one\n";
			Log::Debug << "Json error: " << std::string (e.what ()) << "\n";
			SaveTextureList ();
		}
	}
	else
	{
		Log::Debug << "Texture List file didn't exist, creating one";
		SaveTextureList ();
	}

	try
	{
		Log::Debug << textureResources.size () << " textures loaded\n";
		int count = 0;
		for (nlohmann::json::iterator it = j.begin (); it != j.end (); ++it)
		{
			auto tex = from_json_TexResource (*it);
			textureResources[tex.id] = tex;
			LoadTextureFromFile (tex.id);
			Log::Debug << "Tex " << tex.id << " with name " << tex.name << " dimensions width "
			           << tex.dataDescription.width << " height " << tex.dataDescription.height << "\n";
			count++;
		}
		id_counter = count;
	}
	catch (nlohmann::json::parse_error& e)
	{
		Log::Debug << "Error loading texture list " << e.what () << "\n";
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
		Log::Error << e.what () << "\n";
	}
	outFile.close ();
}

void Manager::LoadTextureFromFile (TexID id)
{

	auto& texRes = textureResources.at (id);
	textureData.push_back (std::make_unique<TexData> (texRes.dataDescription));
	texRes.SetDataPtr (&(*textureData.back ()));

	int desiredChannels = texRes.dataDescription.channels;

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

		if (path.c_str () == "")
		{
			Log::Error << "Path not set!\n";
		}
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels;
		pixels = stbi_load (path.c_str (), &texWidth, &texHeight, &texChannels, 4);

		if (pixels == nullptr)
		{

			Log::Error << "Image failed to load! Was the name correct? " << path.c_str () << "\n";
		}
		// else if (desiredChannels != texChannels) {
		//	Log::Error << "Image couldn't load desired channel of " << desiredChannels
		//		<< " but only " << texChannels << " channels\n";
		//}
		else
		{
			std::memcpy (textureData.back ()->GetDataPtr () + i * texWidth * texHeight * 4,
			    pixels,
			    sizeof (std::byte) * texWidth * texHeight * 4);


			stbi_image_free (pixels);
		}
	}
}

TexID CreateNewTextureFromByteArray (int width, int height, std::byte* data) { return 0; }

TexResource Manager::GetTexResourceByID (TexID id)
{
	std::lock_guard<std::mutex> lk (lock);
	return textureResources.at (id);
}

TexID Manager::GetTexIDByName (std::string s)
{
	std::lock_guard<std::mutex> lk (lock);
	for (auto const& [key, val] : textureResources)
	{
		if (val.name == s) return key;
	}
	throw std::runtime_error ("No matching texture found with name " + s);
}

} // namespace Resource::Texture
