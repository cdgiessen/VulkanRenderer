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

	cml::mat4f get_proj_mat ();
	cml::mat4f get_view_mat ();
	cml::mat4f get_proj_view_mat ();
	cml::vec3f get_position ();
	cml::quatf get_rotation ();
	float get_fov ();
	float get_aspect_ratio ();
	float get_view_size ();
	float get_clip_near ();
	float get_clip_far ();

	void set_position (cml::vec3f position);
	void set_rotation (cml::quatf rotation);
	void set_fov (float fov);
	void set_aspect_ratio (float aspect);
	void set_view_size (float size);
	void set_clip_near (float near);
	void set_clip_far (float far);

	private:
	CameraType type;
	float fov = 1.0f; // radians
	float aspect = 1.0f;
	float size = 1.0f;

	float clip_near = 0.01f;
	float clip_far = 10000.0f;

	cml::vec3f position = cml::vec3f (0, 0, 0);
	cml::quatf rotation = cml::quatf (1, 0, 0, 0);

	cml::mat4f mat_proj_view;
	cml::mat4f mat_view;
	cml::mat4f mat_proj;

	bool view_mat_dirty = true;
	bool proj_mat_dirty = true;

	friend class RenderCameras;
	bool is_active = false;
};

struct CameraGPUData
{
	cml::mat4f proj_view;
	cml::mat4f view;
	cml::vec3f camera_dir;
	cml::vec3f camera_pos;
};

using ViewCameraID = int;
const uint32_t MaxCameraCount = 32;
class RenderCameras
{
	public:
	RenderCameras (VulkanDevice& device);
	RenderCameras (RenderCameras const& cam) = delete;
	RenderCameras operator= (RenderCameras const& cam) = delete;

	ViewCameraID create (CameraType type, cml::vec3f position, cml::quatf rotation);
	void remove (ViewCameraID id);

	ViewCameraData& get_camera_data (ViewCameraID id);
	void set_camera_data (ViewCameraID id, ViewCameraData const& data);

	void update_gpu_buffer (int index);

	VkDescriptorBufferInfo get_descriptor_info (int index, ViewCameraID id);
	VkDescriptorType get_descriptor_type () { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }

	private:
	void setup_view_camera (ViewCameraID id, CameraType type, cml::vec3f position, cml::quatf rotation);

	ViewCameraID cur_id = 0;
	std::mutex lock;
	std::vector<ViewCameraData> camera_data;

	VulkanDevice& device;
	DoubleBuffer data_buffers;
};