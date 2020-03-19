#include "common/pbr_func.glsl"

layout (set = 3, binding = 0) uniform PBR_Material
{
    sampler2D albedo_tex;
	sampler2D metallic_tex;
	sampler2D roughness_tex;
    sampler2D normal;
} pbr_mat;