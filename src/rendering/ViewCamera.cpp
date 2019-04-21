#include "ViewCamera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

const glm::mat4 depthReverserMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1 };

const glm::vec3 WorldUp{ 0, 1, 0 };

glm::mat4 Camera::ViewMatrix ()
{
	if (isViewMatDirty)
	{
		isViewMatDirty = false;
		glm::vec3 front = glm::normalize (rotation * glm::vec3{ 0, 0, 1 });

		glm::vec3 right = glm::normalize (glm::cross (front, WorldUp));
		glm::vec3 up = glm::normalize (glm::cross (right, front));

		mat_view = glm::lookAt (position, position + front, up);
	}
	return mat_view;
}
glm::mat4 Camera::ProjMatrix ()
{
	if (isProjMatDirty)
	{
		isProjMatDirty = false;
		if (type == CamType::perspective)
		{
			mat_proj = depthReverserMatrix * glm::perspective (fov, aspect, clip_near, clip_far);
		}
		else
		{
			mat_proj = glm::ortho (-size, size, -size, size, clip_near, clip_far);
		}
	}
	return mat_proj;
}

glm::mat4 Camera::ViewProjMatrix () { return ProjMatrix () * ViewMatrix (); }

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

void Camera::Position (glm::vec3 position)
{
	isViewMatDirty = true;
	this->position = position;
}
void Camera::Rotation (glm::quat rotation)
{
	isViewMatDirty = true;
	this->rotation = rotation;
}
