#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct GPU_ProjView
{
	glm::mat4 projView;
};


class Camera
{
	enum CamType
	{
		orthographic,
		perspective
	};

	public:
	// perspective
	Camera (glm::vec3 position, glm::quat rotation, float fov, float aspect)
	: position (position), rotation (rotation), type (CamType::perspective), fov (fov), size (aspect)
	{
	}

	// orthographic
	Camera (glm::vec3 position, glm::quat rotation, float size)
	: position (position), rotation (rotation), type (CamType::orthographic), size (size)
	{
	}


	glm::mat4 ViewMatrix ();
	glm::mat4 ProjMatrix ();
	glm::mat4 ViewProjMatrix ();

	void FieldOfView (float fov);
	void AspectRatio (float aspect);
	void ViewSize (float size);

	void ClipNear (float near);
	void ClipFar (float far);

	void Position (glm::vec3 position);
	void Rotation (glm::quat rotation);

	private:
	CamType const type;
	float fov = 1.0f; // radians
	float aspect = 1.0f;
	float size = 1.0f;

	float clip_near = 0.01f;
	float clip_far = 10000.0f;

	glm::vec3 position = glm::vec3 (0, 0, 0);
	glm::quat rotation = glm::quat (1, 0, 0, 0);

	glm::mat4 mat_viewProj;

	bool isViewMatDirty = false;
	glm::mat4 mat_view;

	bool isProjMatDirty = false;
	glm::mat4 mat_proj;
};
