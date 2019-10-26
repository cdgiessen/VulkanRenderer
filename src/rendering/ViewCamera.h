#pragma once

#include <vector>

#include "SG14/flat_map.h"

#include "cml/cml.h"

class Camera
{
	enum CamType
	{
		orthographic,
		perspective
	};

	public:
	// perspective
	Camera (cml::vec3f position, cml::quatf rotation, float fov, float aspect);

	// orthographic
	Camera (cml::vec3f position, cml::quatf rotation, float size);

	cml::mat4f ViewMat ();
	cml::mat4f ProjMat ();
	cml::mat4f ViewProjMat ();

	void FOV (float fov);
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
	cml::mat4f mat_view;
	cml::mat4f mat_proj;

	bool isViewMatDirty = false;
	bool isProjMatDirty = false;
};

using CameraID = int;

class CameraManager
{
	public:
	CameraManager ();

	CameraID Create ();
	Camera& Get (CameraID id);
	void Delete (CameraID id);

	private:
	CameraID i = 0;
	stdext::flat_map<CameraID, Camera> cameras;
};