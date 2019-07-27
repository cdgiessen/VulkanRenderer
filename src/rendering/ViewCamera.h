#pragma once

#include "cml/cml.h"

struct GPU_ProjView
{
	cml::mat4f projView;
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
	Camera (cml::vec3f position, cml::quatf rotation, float fov, float aspect)
	: position (position), rotation (rotation), type (CamType::perspective), fov (fov), size (aspect)
	{
	}

	// orthographic
	Camera (cml::vec3f position, cml::quatf rotation, float size)
	: position (position), rotation (rotation), type (CamType::orthographic), size (size)
	{
	}


	cml::mat4f ViewMatrix ();
	cml::mat4f ProjMatrix ();
	cml::mat4f ViewProjMatrix ();

	void FieldOfView (float fov);
	void AspectRatio (float aspect);
	void ViewSize (float size);

	void ClipNear (float near);
	void ClipFar (float far);

	void Position (cml::vec3f position);
	void Rotation (cml::quatf rotation);

	private:
	CamType const type;
	float fov = 1.0f; // radians
	float aspect = 1.0f;
	float size = 1.0f;

	float clip_near = 0.01f;
	float clip_far = 10000.0f;

	cml::vec3f position = cml::vec3f (0, 0, 0);
	cml::quatf rotation = cml::quatf (1, 0, 0, 0);

	cml::mat4f mat_viewProj;

	bool isViewMatDirty = false;
	cml::mat4f mat_view;

	bool isProjMatDirty = false;
	cml::mat4f mat_proj;
};
