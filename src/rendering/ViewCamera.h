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

struct GPU_CameraData
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec4 cameraDir;
	glm::vec4 cameraPos;
};


class ViewCamera
{

	struct Orthographic
	{
		float size = 1.0f;
	};

	struct Perspective
	{
		float fov = 1.0f;
	};

	struct ClipPlanes
	{
		float clip_near = 0.01f;
		float clip_far = 10000.0f;
	} clipPlanes;

	// std::variant<ViewCamera::Orthographic, ViewCamera::Perspective> projectionType = ViewCamera::Orthographic{};



	glm::vec3 pos = glm::vec3 (0, 0, 0);
	glm::vec3 scale = glm::vec3 (1, 1, 1);
	glm::quat rot = glm::quat (1, 0, 0, 0);

	GPU_CameraData CameraData ();
};

class ViewSurface
{
	ViewCamera& cam;
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

const glm::mat4 depthReverserMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1 };


class ViewManager
{
	int AddCamera ();



	std::unique_ptr<VulkanBufferUniformPersistant> camera_data;
};