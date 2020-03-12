#include "gltf.h"


#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "core/Logger.h"

using namespace nlohmann;

namespace gltf
{

// Enums
void from_json (json const& j, ComponentType& p)
{
	switch (j.get<uint32_t> ())
	{
		case (5120): p = ComponentType::byte; break;
		case (5121): p = ComponentType::unsigned_byte; break;
		case (5122): p = ComponentType::short16; break;
		case (5123): p = ComponentType::unsigned_short; break;
		case (5125): p = ComponentType::unsigned_int; break;
		case (5126): p = ComponentType::float32; break;
		default: throw json::parse_error::create (999, 0, "Invalid ComponentType value");
	}
}
NLOHMANN_JSON_SERIALIZE_ENUM (AccessorType,
    {
        { AccessorType::invalid, nullptr },
        { AccessorType::scalar, "SCALAR" },
        { AccessorType::vec2, "VEC2" },
        { AccessorType::vec3, "VEC3" },
        { AccessorType::vec4, "VEC4" },
        { AccessorType::mat2, "MAT2" },
        { AccessorType::mat3, "MAT3" },
        { AccessorType::mat4, "MAT4" },
    })
NLOHMANN_JSON_SERIALIZE_ENUM (AttributeType,
    {
        { AttributeType::invalid, nullptr },
        { AttributeType::POSITION, "POSITION" },
        { AttributeType::NORMAL, "NORMAL" },
        { AttributeType::TANGENT, "TANGENT" },
        { AttributeType::TEXCOORD_0, "TEXCOORD_0" },
        { AttributeType::TEXCOORD_1, "TEXCOORD_1" },
        { AttributeType::COLOR_0, "COLOR_0" },
        { AttributeType::JOINTS_0, "JOINTS_0" },
        { AttributeType::WEIGHTS_0, "WEIGHTS_0" },
    })
void from_json (json const& j, BufferTarget& p)
{
	switch (j.get<uint32_t> ())
	{
		case (34962): p = BufferTarget::ARRAY_BUFFER; break;
		case (34963): p = BufferTarget::ELEMENT_BUFFER_ARRAY; break;
		default: throw json::parse_error::create (999, 0, "Invalid BufferTarget value");
	}
}
NLOHMANN_JSON_SERIALIZE_ENUM (MimeType,
    {
        { MimeType::invalid, nullptr },
        { MimeType::jpeg, "image/jpeg" },
        { MimeType::png, "image/png" },
    })
void from_json (json const& j, Filter& p)
{
	switch (j.get<uint32_t> ())
	{
		case (9728): p = Filter::NEAREST; break;
		case (9729): p = Filter::LINEAR; break;
		case (9984): p = Filter::NEAREST_MIPMAP_NEAREST; break;
		case (9985): p = Filter::NEAREST_MIPMAP_LINEAR; break;
		case (9986): p = Filter::LINEAR_MIPMAP_NEAREST; break;
		case (9987): p = Filter::LINEAR_MIPMAP_LINEAR; break;
		default: throw json::parse_error::create (999, 0, "Invalid Filter value");
	}
}
void from_json (json const& j, Wrap& p)
{
	switch (j.get<uint32_t> ())
	{
		case (33071): p = Wrap::CLAMP_TO_EDGE; break;
		case (33648): p = Wrap::MIRRORED_REPEAT; break;
		case (10497): p = Wrap::REPEAT; break;
		default: throw json::parse_error::create (999, 0, "Invalid Wrap value");
	}
}
NLOHMANN_JSON_SERIALIZE_ENUM (CameraType,
    {
        { CameraType::invalid, nullptr },
        { CameraType::orthographic, "orthographic" },
        { CameraType::perspective, "perspective" },
    })
NLOHMANN_JSON_SERIALIZE_ENUM (AlphaMode,
    {
        { AlphaMode::OPAQUE, "OPAQUE" },
        { AlphaMode::MASK, "MASK" },
        { AlphaMode::BLEND, "BLEND" },
    })
void from_json (json const& j, PrimitiveMode& p)
{
	switch (j.get<uint32_t> ())
	{
		case (0): p = PrimitiveMode::POINTS; break;
		case (1): p = PrimitiveMode::LINES; break;
		case (2): p = PrimitiveMode::LINE_LOOP; break;
		case (3): p = PrimitiveMode::LINE_STRIP; break;
		case (4): p = PrimitiveMode::TRIANGLES; break;
		case (5): p = PrimitiveMode::TRIANGLE_STRIP; break;
		case (6): p = PrimitiveMode::TRIANGLE_FAN; break;
		default: throw json::parse_error::create (999, 0, "Invalid PrimitiveMode value");
	}
}
NLOHMANN_JSON_SERIALIZE_ENUM (TargetPath,
    {
        { TargetPath::invalid, nullptr },
        { TargetPath::translation, "translation" },
        { TargetPath::rotation, "rotation" },
        { TargetPath::scale, "scale" },
        { TargetPath::weights, "weights" },
    })
NLOHMANN_JSON_SERIALIZE_ENUM (Interpolation,
    {
        { Interpolation::linear, "linear" },
        { Interpolation::step, "step" },
        { Interpolation::cubicspline, "cubicspline" },
    })

// Helpers
} // namespace gltf
namespace cml
{
template <typename T> void from_json (json const& j, vec2<T>& p)
{
	if (j.is_array () && j.size () == 2)
	{
		for (int i = 0; i < 2; i++)
			p.set (i, j[i]);
	}
}
template <typename T> void from_json (json const& j, vec3<T>& p)
{
	if (j.is_array () && j.size () == 3)
	{
		for (int i = 0; i < 3; i++)
			p.set (i, j[i]);
	}
}
template <typename T> void from_json (json const& j, vec4<T>& p)
{
	if (j.is_array () && j.size () == 4)
	{
		for (int i = 0; i < 4; i++)
			p.set (i, j[i]);
	}
}
template <typename T> void from_json (json const& j, mat3<T>& p)
{
	if (j.is_array () && j.size () == 9)
	{
		for (int i = 0; i < 9; i++)
			p.set (i, j[i]);
	}
}
template <typename T> void from_json (json const& j, mat4<T>& p)
{
	if (j.is_array () && j.size () == 16)
	{
		for (int i = 0; i < 16; i++)
			p.set (i, j[i]);
	}
}
template <typename T> void from_json (json const& j, quat<T>& p)
{
	if (j.is_array () && j.size () == 4)
	{
		for (int i = 0; i < 4; i++)
			p.set (i, j[i]);
	}
}
} // namespace cml
namespace gltf
{
template <typename T> void get_optional (json const& j, const char* name, std::optional<T>& opt)
{
	if (j.contains (name))
		opt = j.at (name).get<T> ();
	else
		opt = {};
}
template <typename T> void get_optional (json const& j, const char* name, T& opt)
{
	if (j.contains (name)) opt = j.at (name).get<T> ();
}
template <typename T>
void get_optional_vector (json const& j, const char* name, std::vector<T>& opt)
{
	if (j.contains (name)) j.at (name).get_to (opt);
}
// Structs
void from_json (json const& j, SparseValue& p)
{
	j.at ("bufferView").get_to (p.bufferView);
	j.at ("byteOffset").get_to (p.byteOffset);
}
void from_json (json const& j, Indices& p)
{
	j.at ("bufferView").get_to (p.bufferView);
	j.at ("byteOffset").get_to (p.byteOffset);
	j.at ("componentType").get_to (p.type);
}
void from_json (json const& j, Sparse& p)
{
	j.at ("count").get_to (p.count);
	j.at ("indices").get_to (p.indices);
	j.at ("values").get_to (p.values);
}

void from_json (json const& j, Accessor& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "bufferView", p.bufferView);
	j.at ("byteOffset").get_to (p.byteOffset);
	j.at ("componentType").get_to (p.componentType);
	get_optional (j, "normalized", p.normalized);
	j.at ("count").get_to (p.count);
	j.at ("type").get_to (p.accessorType);
	get_optional_vector (j, "max", p.max);
	get_optional_vector (j, "min", p.min);
	get_optional (j, "sparse", p.sparse);
}
void from_json (json const& j, AnimationSampler& p)
{
	j.at ("input").get_to (p.input);
	j.at ("interpolation").get_to (p.interpolation);
	j.at ("output").get_to (p.output);
}
void from_json (json const& j, Target& p)
{
	get_optional (j, "node", p.node);
	j.at ("path").get_to (p.path);
}
void from_json (json const& j, Channel& p)
{
	j.at ("sampler").get_to (p.sampler);
	j.at ("target").get_to (p.target);
}
void from_json (json const& j, Animation& p)
{
	j.at ("name").get_to (p.name);
	j.at ("samplers").get_to (p.samplers);
	j.at ("channels").get_to (p.channels);
}
void from_json (json const& j, Attributes& p)
{
	get_optional (j, "POSITION", p.POSITION);
	get_optional (j, "NORMAL", p.NORMAL);
	get_optional (j, "TANGENT", p.TANGENT);
	get_optional (j, "TEXCOORD_0", p.TEXCOORD_0);
	get_optional (j, "TEXCOORD_1", p.TEXCOORD_1);
	get_optional (j, "COLOR_0", p.COLOR_0);
	get_optional (j, "JOINTS_0", p.JOINTS_0);
	get_optional (j, "WEIGHTS_0", p.WEIGHTS_0);
}
void from_json (json const& j, Primitive& p)
{
	j.at ("attributes").get_to (p.attributes);
	get_optional (j, "indices", p.indices);
	get_optional (j, "material", p.material);
	get_optional (j, "mode", p.mode);
	get_optional_vector (j, "targets", p.morphTargets);
}
void from_json (json const& j, Mesh& p)
{
	get_optional (j, "name", p.name);
	j.at ("primitives").get_to (p.primitives);
	get_optional_vector (j, "weights", p.weights);
}
void from_json (json const& j, BufferView& p)
{
	get_optional (j, "name", p.name);
	j.at ("buffer").get_to (p.buffer);
	get_optional (j, "byteOffset", p.byteOffset);
	j.at ("byteLength").get_to (p.byteLength);
	get_optional (j, "byteStride", p.byteStride);
	get_optional (j, "target", p.bufferTarget);
}
void from_json (json const& j, Buffer& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "uri", p.uri);
	j.at ("byteLength").get_to (p.byteLength);
}
void from_json (json const& j, Skin& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "inverseBindMatrices", p.inverseBindMatrices);
	get_optional (j, "skeleton", p.skeleton);
	j.at ("joints").get_to (p.joints);
}
void from_json (json const& j, Image& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "uri", p.uri);
	get_optional (j, "mimeType", p.type);
	get_optional (j, "bufferView", p.bufferView);
}
void from_json (json const& j, Sampler& p)
{
	get_optional (j, "magFilter", p.magFilter);
	get_optional (j, "minFilter", p.minFilter);
	get_optional (j, "wrapS", p.wrapS);
	get_optional (j, "wrapT", p.wrapT);
}
void from_json (json const& j, Texture& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "sampler", p.sampler);
	get_optional (j, "source", p.source);
}
void from_json (json const& j, TextureReference& p)
{
	get_optional (j, "index ", p.index);
	get_optional (j, "texCoord ", p.texCoord);
	get_optional (j, "scale ", p.scale);
}
void from_json (json const& j, PBRMetallicRoughness& p)
{
	get_optional (j, "baseColorFactor", p.baseColorFactor);
	get_optional (j, "baseColorTexture", p.baseColorTexture);
	get_optional (j, "metallicFactor", p.metallicFactor);
	get_optional (j, "roughnessFactor", p.roughnessFactor);
	get_optional (j, "metallicRoughnessTexture", p.metallicRoughnessTexture);
}
void from_json (json const& j, KHR_materials_pbrSpecularGlossiness& p)
{
	get_optional (j, "diffuseFactor", p.diffuseFactor);
	get_optional (j, "diffuseTexture", p.diffuseTexture);
	get_optional (j, "specularFactor", p.specularFactor);
	get_optional (j, "glossinessFactor", p.glossinessFactor);
	get_optional (j, "specularGlossinessTexture", p.specularGlossinessTexture);
}
void from_json (json const& j, MaterialExtensions& p)
{
	get_optional (j, "KHR_materials_pbrSpecularGlossiness", p.pbrSpecularGlossiness);
}
void from_json (json const& j, Material& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "pbrMetallicRoughness", p.pbrMetallicRoughness);
	get_optional (j, "normalTexture ", p.normalTexture);
	get_optional (j, "occlusionTexture ", p.occlusionTexture);
	get_optional (j, "emissiveTexture ", p.emissiveTexture);
	get_optional (j, "emissiveFactor ", p.emissiveFactor);
	get_optional (j, "alphaMode", p.alphaMode);
	get_optional (j, "alphaCutoff", p.alphaCutoff);
	get_optional (j, "doubleSided", p.doubleSided);
	get_optional (j, "extensions", p.extensions);
}
void from_json (json const& j, Orthographic& p)
{
	j.at ("xmag").get_to (p.xmag);
	j.at ("ymag").get_to (p.ymag);
	j.at ("zfar").get_to (p.zfar);
	j.at ("znear").get_to (p.znear);
}
void from_json (json const& j, Perspective& p)
{
	j.at ("aspectRatio").get_to (p.aspectRatio);
	j.at ("yfov").get_to (p.yfov);
	j.at ("zfar").get_to (p.zfar);
	j.at ("znear").get_to (p.znear);
}
void from_json (json const& j, Camera& p)
{
	get_optional (j, "name", p.name);
	get_optional (j, "orthographic", p.orthographic);
	get_optional (j, "perspective", p.perspective);
	j.at ("type").get_to (p.type);
}
void from_json (json const& j, Node& p)
{
	get_optional (j, "name", p.name);
	get_optional_vector (j, "children", p.children);
	get_optional (j, "skin", p.skin);
	get_optional (j, "mesh", p.mesh);
	get_optional_vector (j, "weights", p.weights);

	if (j.contains ("matrix"))
	{
		cml::mat4f matrix;
		get_optional (j, "matrix", matrix);
		p.transform = matrix;
	}
	else
	{
		cml::vec3f translation;
		cml::quatf rotation;
		cml::vec3f scale = cml::vec3f::one;
		get_optional (j, "translation", translation);
		get_optional (j, "rotation", rotation);
		get_optional (j, "scale", scale);
		p.transform = TRS{ translation, rotation, scale };
	}
}
void from_json (json const& j, Scene& p)
{
	get_optional (j, "name", p.name);
	get_optional_vector (j, "nodes", p.nodes);
}
void from_json (json const& j, Asset& p)
{
	get_optional (j, "copyright", p.copyright);
	get_optional (j, "generator", p.generator);
	j.at ("version").get_to (p.version);
	get_optional (j, "minVersion", p.minVersion);
}
void from_json (json const& j, RawGLTF& p)
{
	j.at ("asset").get_to (p.asset);
	get_optional (j, "scene", p.display_scene);
	get_optional_vector (j, "extensionsUsed", p.extensionsUsed);
	get_optional_vector (j, "extensionsRequired", p.extensionsRequired);
	get_optional_vector (j, "accessors", p.accessors);
	get_optional_vector (j, "animations", p.animations);
	get_optional_vector (j, "buffers", p.buffers);
	get_optional_vector (j, "bufferViews", p.bufferViews);
	get_optional_vector (j, "cameras", p.cameras);
	get_optional_vector (j, "images", p.images);
	get_optional_vector (j, "materials", p.materials);
	get_optional_vector (j, "meshes", p.meshes);
	get_optional_vector (j, "nodes", p.nodes);
	get_optional_vector (j, "samplers", p.samplers);
	get_optional_vector (j, "scenes", p.scenes);
	get_optional_vector (j, "skins", p.skins);
	get_optional_vector (j, "textures", p.textures);
}


std::optional<RawGLTF> parse_gltf_file (std::string name)
{

	std::ifstream i (name);
	json j;
	try
	{
		i >> j;
		RawGLTF jkl = j;
		return j;
	}
	catch (json::exception& e)
	{
		Log.Debug (fmt::format ("Couldn't open gltf: {}", e.what ()));
		return {};
	}
}

} // namespace gltf
