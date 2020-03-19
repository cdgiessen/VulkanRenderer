
layout (set = 0, binding = 1) uniform CameraData
{
	mat4 proj_view;
	mat4 view;
	vec3 camera_dir;
	vec3 camera_pos;
}
cam;