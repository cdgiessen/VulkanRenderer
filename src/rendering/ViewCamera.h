#pragma once

#include <array>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Buffer.h"

struct GPU_ProjView
{
	glm::mat4 projView;
};


struct ClipPlanes
{
	float clip_near = 0.01f;
	float clip_far = 10000.0f;
};

struct CameraTransform
{
	glm::vec3 pos = glm::vec3 (0, 0, 0);
	glm::vec3 scale = glm::vec3 (1, 1, 1);
	glm::quat rot = glm::quat (1, 0, 0, 0);
};

struct PerspectiveCamera
{
	float fov = 1.0f;
	ClipPlanes clipPlanes;

	CameraTransform transform;
};

struct OrthogonalCamera
{
	float size = 1.0f;
	ClipPlanes clipPlanes;

	CameraTransform transform;
};

class ViewSurface
{
	// ViewCamera& cam;
	struct Viewport
	{
		int x = 0, y = 0;
		int width = 1, height = 1;
	} viewport;
	struct Sciossor
	{
		int offsetX = 0, offsetY = 0;
		int width = 1, height = 1;
	} scissor;

	// RenderPass& renderPass;
};

class ViewManager
{
	int AddCamera (std::variant<PerspectiveCamera, OrthogonalCamera> camera);
	int AddSurfaces (ViewSurface surface);


	std::unique_ptr<VulkanBufferUniformPersistant> camera_data;
	std::vector<glm::mat4> projectionMatrices;
};