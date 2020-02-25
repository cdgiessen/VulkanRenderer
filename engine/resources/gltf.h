#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <cml/cml.h>

namespace gltf
{
enum class ComponentType
{
	byte,           // 5120
	unsigned_byte,  // 5121
	short16,        // 5122
	unsigned_short, // 5123
	unsigned_int,   // 5125
	float32         // 5126
};

enum class AccessorType
{
	invalid, // error state
	scalar,  // 1 component
	vec2,    // 2 components
	vec3,    // 3 components
	vec4,    // 4 components
	mat2,    // 4 components
	mat3,    // 9 components
	mat4,    // 16 components
};

enum class AttributeType
{
	invalid,    // error state
	POSITION,   // vec3
	NORMAL,     // vec3
	TANGENT,    // vec4
	TEXCOORD_0, // vec2
	TEXCOORD_1, // vec2
	COLOR_0,    // vec3/vec4
	JOINTS_0,   // vec4
	WEIGHTS_0,  // vec4
};

enum class BufferTarget
{
	ARRAY_BUFFER,        // 34962
	ELEMENT_BUFFER_ARRAY // 34963
};

enum class MimeType
{
	invalid,
	jpeg,
	png
};

enum class Filter
{
	NEAREST,                // 9728
	LINEAR,                 // 9729
	NEAREST_MIPMAP_NEAREST, // 9984
	NEAREST_MIPMAP_LINEAR,  // 9985
	LINEAR_MIPMAP_NEAREST,  // 9986
	LINEAR_MIPMAP_LINEAR    // 9987
};
enum class Wrap
{
	CLAMP_TO_EDGE,   // 33071
	MIRRORED_REPEAT, // 33648
	REPEAT           // 10497
};

enum class CameraType
{
	invalid,
	orthographic,
	perspective
};

enum class AlphaMode
{
	OPAQUE,
	MASK,
	BLEND
};

enum class PrimitiveMode
{
	POINTS,         // 0
	LINES,          // 1
	LINE_LOOP,      // 2
	LINE_STRIP,     // 3
	TRIANGLES,      // 4
	TRIANGLE_STRIP, // 5
	TRIANGLE_FAN    // 6
};

enum class TargetPath
{
	invalid,
	translation,
	rotation,
	scale,
	weights,
};

enum class Interpolation
{
	linear,
	step,
	cubicspline
};

struct BufferView
{
	std::string name;
	uint32_t buffer = 0;
	uint32_t byteOffset = 0;
	uint32_t byteLength = 0;
	uint32_t byteStride = 0;
	BufferTarget bufferTarget;
};
struct Buffer
{
	std::string name;
	uint32_t byteLength = 0;
	std::string uri;
};


struct Target
{
	TargetPath path;
	std::optional<uint32_t> node = std::nullopt;
};

struct Channel
{
	uint32_t sampler;
	Target target;
};

struct Indices
{
	uint32_t bufferView;
	uint32_t byteOffset = 0;
	ComponentType type;
};

struct SparseValue
{
	uint32_t bufferView;
	uint32_t byteOffset = 0;
};

struct Sparse
{
	uint32_t count;
	Indices indices;
	std::vector<SparseValue> values;
};

struct AnimationSampler
{
	uint32_t input;
	uint32_t output;
	Interpolation interpolation = Interpolation::linear;
};

struct Animation
{
	std::string name;
	std::vector<Channel> channels;
	std::vector<AnimationSampler> samplers;
};

struct Skin
{
	std::string name;
	std::optional<uint32_t> inverseBindMatrices = std::nullopt;
	std::optional<uint32_t> skeleton;
	std::vector<uint32_t> joints;
};

struct Accessor
{
	std::string name;
	std::optional<uint32_t> bufferView = std::nullopt;
	uint32_t byteOffset = 0;
	ComponentType componentType;
	bool normalized = false;
	uint32_t count = 0;
	AccessorType accessorType;
	std::vector<float> min;
	std::vector<float> max;
	std::optional<Sparse> sparse = std::nullopt;
};
struct Attributes
{
	std::optional<uint32_t> POSITION;   // vec3
	std::optional<uint32_t> NORMAL;     // vec3
	std::optional<uint32_t> TANGENT;    // vec4
	std::optional<uint32_t> TEXCOORD_0; // vec2
	std::optional<uint32_t> TEXCOORD_1; // vec2
	std::optional<uint32_t> COLOR_0;    // vec3/vec4
	std::optional<uint32_t> JOINTS_0;   // vec4
	std::optional<uint32_t> WEIGHTS_0;  // vec4
};

struct Primitive
{
	Attributes attributes;
	uint32_t indices = 0;
	uint32_t material = 0;
	PrimitiveMode mode = PrimitiveMode::TRIANGLES;
	std::vector<Target> morphTargets;
};

struct Mesh
{
	std::string name;
	std::vector<Primitive> primitives;
	std::vector<float> weights;
};

struct Image
{
	std::string name;
	std::string uri;
	MimeType type;
	std::optional<uint32_t> bufferView = std::nullopt;
};

struct Sampler
{
	Filter magFilter;
	Filter minFilter;
	Wrap wrapS;
	Wrap wrapT;
};

struct Texture
{
	std::string name;
	std::optional<uint32_t> sampler = std::nullopt;
	std::optional<uint32_t> source = std::nullopt;
};

struct TextureReference
{
	uint32_t index = 0;
	uint32_t texCoord = 0;
	float scale = 0;
};

struct PBRMetallicRoughness
{
	cml::vec4f baseColorFactor = cml::vec4f::one;
	std::optional<TextureReference> baseColorTexture = std::nullopt;
	float metallicFactor = 1;
	float roughnessFactor = 1;
	std::optional<TextureReference> metallicRoughnessTexture = std::nullopt;
};

struct Material
{
	std::string name;
	std::optional<PBRMetallicRoughness> pbrMetallicRoughness = std::nullopt;
	std::optional<TextureReference> normalTexture = std::nullopt;
	std::optional<TextureReference> occlusionTexture = std::nullopt;
	std::optional<TextureReference> emissiveTexture = std::nullopt;
	cml::vec3f emissiveFactor = cml::vec3f::one;
	AlphaMode alphaMode = AlphaMode::OPAQUE;
	float alphaCutoff = 0.5f;
	bool doubleSided = false;
};

struct Orthographic
{
	float xmag;
	float ymag;
	float zfar;
	float znear;
};

struct Perspective
{
	float aspectRatio;
	float yfov;
	float zfar;
	float znear;
};

struct Camera
{
	std::string name;
	Orthographic orthographic;
	Perspective perspective;
	CameraType type;
};

struct TRS
{
	cml::vec3f translation;
	cml::quatf rotation;
	cml::vec3f scale = cml::vec3f::one;
};

struct Node
{
	std::string name;
	std::variant<TRS, cml::mat4f> transform;
	std::optional<Camera> camera = std::nullopt;
	std::optional<uint32_t> skin = std::nullopt;
	std::optional<uint32_t> mesh = std::nullopt;
	std::vector<float> weights;
	std::vector<uint32_t> children;
};

struct Scene
{
	std::string name;
	std::vector<uint32_t> nodes;
};

struct Asset
{
	std::string copyright;
	std::string generator;
	std::string version;
	std::string minVersion;
};

struct GLTF
{
	Asset asset;
	int32_t display_scene = -1; //-1 means display nothing
	std::vector<std::string> extensionsUsed;
	std::vector<std::string> extensionsRequired;
	std::vector<Accessor> accessors;
	std::vector<Animation> animations;
	std::vector<Buffer> buffers;
	std::vector<BufferView> bufferViews;
	std::vector<Camera> cameras;
	std::vector<Image> images;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Node> nodes;
	std::vector<Sampler> samplers;
	std::vector<Scene> scenes;
	std::vector<Skin> skins;
	std::vector<Texture> textures;
};

void load_gltf (std::string name);

} // namespace gltf