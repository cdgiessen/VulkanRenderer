#include "ViewCamera.h"

const cml::mat4f depthReverserMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1 };

const cml::vec3f WorldUp{ 0, 1, 0 };

cml::mat4f Camera::ViewMatrix ()
{
	if (isViewMatDirty)
	{
		isViewMatDirty = false;
		cml::vec3f front = cml::vec3f::forward;
		// cml::vec3f front = cml::normalize (rotation * cml::vec3f{ 0, 0, 1 });

		cml::vec3f right = cml::normalize (cml::cross (front, WorldUp));
		cml::vec3f up = cml::normalize (cml::cross (right, front));

		mat_view = cml::lookAt (position, position + front, up);
	}
	return mat_view;
}
cml::mat4f Camera::ProjMatrix ()
{
	if (isProjMatDirty)
	{
		isProjMatDirty = false;
		if (type == CamType::perspective)
		{
			mat_proj = depthReverserMatrix * cml::perspective (fov, aspect, clip_near, clip_far);
		}
		else
		{
			mat_proj = cml::ortho (-size, size, -size, size, clip_near, clip_far);
		}
	}
	return mat_proj;
}

cml::mat4f Camera::ViewProjMatrix () { return ProjMatrix () * ViewMatrix (); }

void Camera::FieldOfView (float fov)
{
	isProjMatDirty = true;
	this->fov = fov;
}
void Camera::AspectRatio (float aspect)
{
	isProjMatDirty = true;
	this->aspect = aspect;
}
void Camera::ViewSize (float size)
{
	isProjMatDirty = true;
	this->size = size;
}

void Camera::ClipNear (float near)
{
	isProjMatDirty = true;
	this->clip_near = near;
}
void Camera::ClipFar (float far)
{
	isProjMatDirty = true;
	this->clip_far = far;
}

void Camera::Position (cml::vec3f position)
{
	isViewMatDirty = true;
	this->position = position;
}
void Camera::Rotation (cml::quatf rotation)
{
	isViewMatDirty = true;
	this->rotation = rotation;
}
