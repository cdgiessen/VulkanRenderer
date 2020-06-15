#include "ViewCamera.h"

#include "rendering/backend/Device.h"

const cml::vec3f WorldUp = cml::vec3f::up;

void ViewCameraData::Setup (CameraType type, cml::vec3f position, cml::quatf rotation)
{
	this->type = type;
	this->position = position;
	this->rotation = rotation;
	proj_mat_dirty = true;
	view_mat_dirty = true;
}

cml::mat4f ViewCameraData::get_view_mat ()
{
	if (view_mat_dirty)
	{
		view_mat_dirty = false;
		cml::vec3f front = cml::vec3f::forward;
		// cml::vec3f front = cml::normalize (rotation * cml::vec3f{ 0, 0, 1 });

		cml::vec3f right = cml::normalize (cml::cross (front, WorldUp));
		cml::vec3f up = cml::normalize (cml::cross (right, front));

		mat_view = cml::lookAt (position, position + front, up);
	}
	return mat_view;
}
cml::mat4f ViewCameraData::get_proj_mat ()
{
	if (proj_mat_dirty)
	{
		proj_mat_dirty = false;
		if (type == CameraType::perspective)
		{
			mat_proj = cml::perspective (fov, aspect, clip_near, clip_far);
		}
		else
		{
			mat_proj = cml::ortho (-size, size, -size, size, clip_near, clip_far);
		}
	}
	return mat_proj;
}

cml::mat4f ViewCameraData::get_proj_view_mat ()
{
	if (proj_mat_dirty || view_mat_dirty)
	{
		mat_proj_view = get_proj_mat () * get_view_mat ();
	}
	return mat_proj_view;
}

cml::vec3f ViewCameraData::get_position () { return position; }
cml::quatf ViewCameraData::get_rotation () { return rotation; }
float ViewCameraData::get_fov () { return fov; }
float ViewCameraData::get_aspect_ratio () { return aspect; }
float ViewCameraData::get_view_size () { return size; }
float ViewCameraData::get_clip_near () { return clip_near; }
float ViewCameraData::get_clip_far () { return clip_far; }

void ViewCameraData::set_position (cml::vec3f position)
{
	view_mat_dirty = true;
	this->position = position;
}
void ViewCameraData::set_rotation (cml::quatf rotation)
{
	view_mat_dirty = true;
	this->rotation = rotation;
}

void ViewCameraData::set_fov (float fov)
{
	proj_mat_dirty = true;
	this->fov = fov;
}
void ViewCameraData::set_aspect_ratio (float aspect)
{
	proj_mat_dirty = true;
	this->aspect = aspect;
}
void ViewCameraData::set_view_size (float size)
{
	proj_mat_dirty = true;
	this->size = size;
}

void ViewCameraData::set_clip_near (float near)
{
	proj_mat_dirty = true;
	clip_near = near;
}
void ViewCameraData::set_clip_far (float far)
{
	proj_mat_dirty = true;
	clip_far = far;
}

RenderCameras::RenderCameras (VulkanDevice& device)
: device (device), data_buffers (device, uniform_details (sizeof (CameraGPUData) * MaxCameraCount))
{
	camera_data.resize (MaxCameraCount);
};

ViewCameraID RenderCameras::create (CameraType type, cml::vec3f position, cml::quatf rotation)
{
	std::lock_guard lg (lock);
	for (ViewCameraID i = 0; i < camera_data.size (); i++)
	{
		if (camera_data.at (i).is_active == false)
		{
			camera_data.at (i).is_active = true;
			camera_data.at (i).Setup (type, position, rotation);
			return i;
		}
	}
	return -1; // assume theres enough cameras available
}
void RenderCameras::remove (ViewCameraID id)
{
	std::lock_guard lg (lock);

	camera_data.at (id).is_active = false;
}


ViewCameraData& RenderCameras::get_camera_data (ViewCameraID id)
{
	std::lock_guard lg (lock);
	return camera_data.at (id);
}
void RenderCameras::set_camera_data (ViewCameraID id, ViewCameraData const& data)
{
	std::lock_guard lg (lock);
	camera_data.at (id) = data;
}

void RenderCameras::update_gpu_buffer (int index)
{
	std::lock_guard lg (lock);
	std::vector<CameraGPUData> data;
	data.reserve (MaxCameraCount);
	for (auto& cam : camera_data)
	{
		CameraGPUData gpu_cam;
		gpu_cam.proj_view = cam.get_proj_view_mat ();
		gpu_cam.view = cam.get_view_mat ();
		gpu_cam.camera_pos = cam.get_position ();
		// TODO
		gpu_cam.camera_dir = cml::vec3f (); // cam.get_rotation (); ???

		data.push_back (gpu_cam);
	}
	data_buffers.Write ().copy_to_buffer (data);
}

void RenderCameras::setup_view_camera (ViewCameraID id, CameraType type, cml::vec3f position, cml::quatf rotation)
{
	camera_data.at (id).type = type;
	camera_data.at (id).position = position;
	camera_data.at (id).rotation = rotation;
	camera_data.at (id).proj_mat_dirty = true;
	camera_data.at (id).view_mat_dirty = true;
}