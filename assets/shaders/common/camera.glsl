
layout (set = 0, binding = 1) uniform CameraData
{
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
}
cam;