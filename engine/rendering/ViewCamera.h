#pragma once

#include <mutex>
#include <vector>

#include "cml/cml.h"

#include "rendering/backend/Buffer.h"

class VulkanDevice;

enum CameraType
{
	orthographic,
	perspective
};

class ViewCameraData
{
	public:
	ViewCameraData () = default;

	void Setup (CameraType type, cml::vec3f position, cml::quatf rotation);

	cml::mat4f GetProjMat ();
	cml::mat4f GetViewMat ();
	cml::mat4f GetProjViewMat ();
	cml::vec3f GetPosition ();
	cml::quatf GetRotation ();
	float GetFov ();
	float GetAspectRatio ();
	float GetViewSize ();
	float GetClipNear ();
	float GetClipFar ();

	void SetPosition (cml::vec3f position);
	void SetRotation (cml::quatf rotation);
	void SetFOV (float fov);
	void SetAspectRatio (float aspect);
	void SetViewSize (float size);
	void SetClipNear (float near);
	void SetClipFar (float far);

	private:
	CameraType type;
	float fov = 1.0f; // radians
	float aspect = 1.0f;
	float size = 1.0f;

	float clip_near = 0.01f;
	float clip_far = 10000.0f;

	cml::vec3f position = cml::vec3f (0, 0, 0);
	cml::quatf rotation = cml::quatf (1, 0, 0, 0);

	cml::mat4f mat_projView;
	cml::mat4f mat_view;
	cml::mat4f mat_proj;

	bool isViewMatDirty = true;
	bool isProjMatDirty = true;

	friend class ViewCameraManager;
	bool is_active = false;
};

struct CameraGPUData
{
	cml::mat4f projView;
	cml::mat4f view;
	cml::vec3f cameraDir;
	cml::vec3f cameraPos;
};

using ViewCameraID = int;
const uint32_t MaxCameraCount = 32;
class ViewCameraManager
{
	public:
	ViewCameraManager (VulkanDevice& device);
	ViewCameraManager (ViewCameraManager const& cam) = delete;
	ViewCameraManager operator= (ViewCameraManager const& cam) = delete;

	ViewCameraID Create (CameraType type, cml::vec3f position, cml::quatf rotation);
	void Delete (ViewCameraID id);

	ViewCameraData& GetCameraData (ViewCameraID id);

	void UpdateGPUBuffer (int index);

	VkDescriptorBufferInfo GetDescriptorInfo (int index, ViewCameraID id);
	VkDescriptorType GetDescriptorType () { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }

	private:
	void SetupViewCamera (ViewCameraID id, CameraType type, cml::vec3f position, cml::quatf rotation);

	ViewCameraID cur_id = 0;
	std::mutex lock;
	std::vector<ViewCameraData> camera_data;

	VulkanDevice& device;
	DoubleBuffer data_buffers;
};