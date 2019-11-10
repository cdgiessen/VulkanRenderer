#include "ViewCamera.h"

#include "Device.h"

const cml::vec3f WorldUp{ 0, 1, 0 };

void ViewCameraData::Setup (CameraType type, cml::vec3f position, cml::quatf rotation)
{
	type = type;
	position = position;
	rotation = rotation;
	isProjMatDirty = true;
	isViewMatDirty = true;
}

cml::mat4f ViewCameraData::GetViewMat ()
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
cml::mat4f ViewCameraData::GetProjMat ()
{
	if (isProjMatDirty)
	{
		isProjMatDirty = false;
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

cml::mat4f ViewCameraData::GetProjViewMat ()
{
	if (isProjMatDirty || isViewMatDirty)
	{
		mat_projView = GetProjMat () * GetViewMat ();
	}
	return mat_projView;
}

cml::vec3f ViewCameraData::GetPosition () { return position; }
cml::quatf ViewCameraData::GetRotation () { return rotation; }
float ViewCameraData::GetFov () { return fov; }
float ViewCameraData::GetAspectRatio () { return aspect; }
float ViewCameraData::GetViewSize () { return size; }
float ViewCameraData::GetClipNear () { return clip_near; }
float ViewCameraData::GetClipFar () { return clip_far; }

void ViewCameraData::SetPosition (cml::vec3f position)
{
	isViewMatDirty = true;
	position = position;
}
void ViewCameraData::SetRotation (cml::quatf rotation)
{
	isViewMatDirty = true;
	rotation = rotation;
}

void ViewCameraData::SetFOV (float fov)
{
	isProjMatDirty = true;
	fov = fov;
}
void ViewCameraData::SetAspectRatio (float aspect)
{
	isProjMatDirty = true;
	aspect = aspect;
}
void ViewCameraData::SetViewSize (float size)
{
	isProjMatDirty = true;
	size = size;
}

void ViewCameraData::SetClipNear (float near)
{
	isProjMatDirty = true;
	clip_near = near;
}
void ViewCameraData::SetClipFar (float far)
{
	isProjMatDirty = true;
	clip_far = far;
}

ViewCameraManager::ViewCameraManager (VulkanDevice& device)
: device (device),
  data_buffers (device, uniform_details (sizeof (CameraGPUData) * MaxCameraCount)){


  };

ViewCameraID ViewCameraManager::Create (CameraType type, cml::vec3f position, cml::quatf rotation)
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
void ViewCameraManager::Delete (ViewCameraID id)
{
	std::lock_guard lg (lock);

	camera_data.at (id).is_active = false;
}


ViewCameraData& ViewCameraManager::GetCameraData (ViewCameraID id) { return camera_data.at (id); }


void ViewCameraManager::UpdateGPUBuffer (int index)
{
	std::vector<CameraGPUData> data;
	data.reserve (MaxCameraCount);
	for (auto& cam : camera_data)
	{
		CameraGPUData gpu_cam;
		gpu_cam.projView = cam.GetProjViewMat ();
		gpu_cam.view = cam.GetViewMat ();
		gpu_cam.cameraPos = cam.GetPosition ();
		// TODO
		gpu_cam.cameraDir = cml::vec3f (); // cam.GetRotation (); ???

		data.push_back (gpu_cam);
	}
	data_buffers.Write ().CopyToBuffer (data);
}

void ViewCameraManager::SetupViewCamera (ViewCameraID id, CameraType type, cml::vec3f position, cml::quatf rotation)
{
	camera_data.at (id).type = type;
	camera_data.at (id).position = position;
	camera_data.at (id).rotation = rotation;
	camera_data.at (id).isProjMatDirty = true;
	camera_data.at (id).isViewMatDirty = true;
}